/*
 * Copyright (C) Citrix Systems Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 only. with the special
 * exception on linking described in file LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 */

#include <errno.h>
#include <malloc.h>
#include <pwd.h>
#include <shadow.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEVELOPMENT
#define ETC_PASSWD   "passwd"
#define TMP_PASSWD   "passwd.XXXXXX"
#define ETC_SPASSWD  "shadow"
#define TMP_SPASSWD  "shadow.XXXXXX"
#else
#define ETC_PASSWD   "/etc/passwd"
#define TMP_PASSWD   "/etc/passwd.XXXXXX"
#define ETC_SPASSWD  "/etc/shadow"
#define TMP_SPASSWD  "/etc/shadow.XXXXXX"
#endif

#define BUFLEN 4096

char           *
unixpwd_get(const char *user)
{
    struct spwd     spw,
                   *sp;
    struct passwd   pwd,
                   *pw;
    char            buf[BUFLEN];

    if (getspnam_r(user, &spw, buf, BUFLEN, &sp) == 0 && sp)
        return strdup(sp->sp_pwdp);
    if (getpwnam_r(user, &pwd, buf, BUFLEN, &pw) == 0 && pw)
        return strdup(pw->pw_passwd);
    return NULL;
}

int
unixpwd_setpwd(const char *user, char *password)
{

    struct passwd   pwd,
                   *pw;
    char            buf[BUFLEN];
    int             tmp;
    FILE           *tmp_file;
    static char     tmp_name[] = TMP_PASSWD;
    struct stat     statbuf;
    int             rc;
    int             updated = 0;

    tmp = mkstemp(tmp_name);
    if (!tmp)
        return errno;
    if (stat(ETC_PASSWD, &statbuf) != 0) {
        close(tmp);
        return errno;
    }
    if (fchown(tmp, statbuf.st_uid, statbuf.st_gid) != 0) {
        close(tmp);
        return errno;
    }
    if (fchmod(tmp, statbuf.st_mode) != 0) {
        close(tmp);
        return errno;
    }
    tmp_file = fdopen(tmp, "w");
    if (!tmp_file) {
        close(tmp);
        return errno;
    }

    setpwent();
    while (1) {
        rc = getpwent_r(&pwd, buf, BUFLEN, &pw);
        if (rc != 0 || !pw)
            break;
        if (!strcmp(user, pw->pw_name)) {
            pw->pw_passwd = password;
            updated++;
        }
        putpwent(pw, tmp_file);
    }
    endpwent();

    fclose(tmp_file);
    if (rc != ENOENT)
        return rc;
    if (!updated)
        return EINVAL;
    if (rename(tmp_name, ETC_PASSWD) != 0)
        return errno;
    return 0;
}


int
unixpwd_setspw(const char *user, char *password)
{

    struct spwd     spw,
                   *sp;
    char            buf[BUFLEN];
    int             tmp;
    FILE           *tmp_file;
    static char     tmp_name[] = TMP_SPASSWD;
    struct stat     statbuf;
    int             rc;
    int             updated = 0;

    tmp = mkstemp(tmp_name);
    if (!tmp)
        return errno;
    if (stat(ETC_SPASSWD, &statbuf) != 0) {
        close(tmp);
        return errno;
    }
    if (fchown(tmp, statbuf.st_uid, statbuf.st_gid) != 0) {
        close(tmp);
        return errno;
    }
    if (fchmod(tmp, statbuf.st_mode) != 0) {
        close(tmp);
        return errno;
    }
    tmp_file = fdopen(tmp, "w");
    if (!tmp_file) {
        close(tmp);
        return errno;
    }
    if (lckpwdf() != 0) {
        close(tmp);
        return ENOLCK;
    }

    setspent();
    while (1) {
        rc = getspent_r(&spw, buf, BUFLEN, &sp);
        if (rc != 0 || !sp)
            break;
        if (!strcmp(user, sp->sp_namp)) {
            sp->sp_pwdp = password;
            updated++;
        }
        putspent(sp, tmp_file);
    }
    endspent();

    fclose(tmp_file);
    if (rc != ENOENT) {
        ulckpwdf();
        return rc;
    }
    if (!updated) {
        ulckpwdf();
        return EINVAL;
    }
    if (rename(tmp_name, ETC_SPASSWD) != 0) {
        ulckpwdf();
        return errno;
    }
    ulckpwdf();
    return 0;
}

char           *
unixpwd_unshadow(void)
{
    struct spwd     spw,
                   *sp;
    struct passwd   pwd,
                   *pw;
    char            pwbuf[BUFLEN];
    char            spbuf[BUFLEN];

    char           *buf;
    int             size,
                    cur;

    size = 1024;
    cur = 0;
    buf = malloc(size);
    if (!buf) {
        return NULL;
    }

    setpwent();
    while (1) {
        char            tmp[BUFLEN];
        int             written;

        if (getpwent_r(&pwd, pwbuf, BUFLEN, &pw) != 0)
            break;
        getspnam_r(pw->pw_name, &spw, spbuf, BUFLEN, &sp);

        written = snprintf(tmp, BUFLEN, "%s:%s:%d:%d:%s:%s\n",
                           pw->pw_name,
                           sp ? sp->sp_pwdp : pw->pw_passwd,
                           pw->pw_uid,
                           pw->pw_gid,
                           pw->pw_gecos, pw->pw_dir, pw->pw_shell);
        if (written >= BUFLEN) {
            endpwent();
            free(buf);
            return NULL;
        }
        while (cur + written > size) {
            size = size << 1;
            buf = realloc(buf, size);
            if (!buf) {
                endpwent();
                return NULL;
            }
        }
        strncpy(buf + cur, tmp, size - cur);
        cur += written;
    }
    endpwent();

    return buf;
}