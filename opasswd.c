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

#ifndef MAIN
#include <caml/alloc.h>
#include <caml/mlvalues.h>
#include <caml/fail.h>
#include <caml/callback.h>
#include <caml/memory.h>
#endif

#define DEVELOPMENT

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
/*
 * getpw: return the encrypted password for the user. This memory must
 * be freed by the caller. On error and when no password entry is found,
 * set errno and return NULL.
 */

static char    *
getpwd(const char *user)
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

/*
 * setpwd set password entry for a user
 */

static int
setpwd(char *user, char *password)
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
    if (stat(ETC_PASSWD, &statbuf) != 0)
        return errno;
    if (fchown(tmp, statbuf.st_uid, statbuf.st_gid) != 0)
        return errno;
    if (fchmod(tmp, statbuf.st_mode) != 0)
        return errno;
    tmp_file = fdopen(tmp, "w");
    if (!tmp_file)
        return errno;

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


/*
 * setspw - set shadow password for user
 */
static int
setspw(char *user, char *password)
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
    if (stat(ETC_SPASSWD, &statbuf) != 0)
        return errno;
    if (fchown(tmp, statbuf.st_uid, statbuf.st_gid) != 0)
        return errno;
    if (fchmod(tmp, statbuf.st_mode) != 0)
        return errno;
    tmp_file = fdopen(tmp, "w");
    if (!tmp_file)
        return errno;
    if (lckpwdf() != 0)
        return ENOLCK;

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

/*
 * unshadow - return /etc/passwd as a string but with password entries from
 * the shadow password file if it has a corresponding entry. The memory
 * returned by this function must be freed by the caller.
 */
static char    *
unshadow(void)
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

#ifndef MAIN
CAMLprim        value
caml_getpwd(value caml_user)
{
    CAMLparam1(caml_user);
    char           *user;
    char           *passwd;
    CAMLlocal1(pw);

    user = String_val(caml_user);
    passwd = getpwd(user);
    if (passwd == NULL && errno != 0)
        caml_failwith(strerror(errno));
    if (passwd == NULL)
        caml_failwith("unspecified error in caml_getpwd()");

    pw = caml_copy_string(passwd);
    free(passwd);
    CAMLreturn(pw);
}

CAMLprim        value
caml_setpwd(value caml_user, value caml_password)
{
    CAMLparam2(caml_user, caml_password);
    char           *user,
                   *password;
    int             rc;

    user = String_val(caml_user);
    password = String_val(caml_password);
    rc = setpwd(user, password);
    if (rc != 0)
        caml_failwith(strerror(rc));
    CAMLreturn(Val_unit);
}

CAMLprim        value
caml_setspw(value caml_user, value caml_password)
{
    CAMLparam2(caml_user, caml_password);
    char           *user,
                   *password;
    int             rc;

    user = String_val(caml_user);
    password = String_val(caml_password);
    rc = setspw(user, password);
    if (rc != 0)
        caml_failwith(strerror(rc));
    CAMLreturn(Val_unit);
}

CAMLprim        value
caml_unshadow(void)
{
    CAMLparam0();
    char           *passwords;
    CAMLlocal1(str);

    passwords = unshadow();
    if (passwords == NULL && errno != 0)
        caml_failwith(strerror(errno));
    if (passwords == NULL)
        caml_failwith("unspecified error in caml_unshadow()");

    str = caml_copy_string(passwords);
    free(passwords);
    CAMLreturn(str);
}
#endif

#ifdef MAIN
int
main(int argc, char **argv)
{
    int             rc;
    char           *pw;
    char           *buf;

    switch (argc) {
    case 1:
        buf = unshadow();
        if (buf) {
            puts(buf);
            free(buf);
        } else {
            fprintf(stderr, "can't unshadow\n");
        }
        break;

    case 2:
        pw = getpwd(argv[1]);
        if (pw) {
            printf("%s: %s\n", argv[1], pw);
            free(pw);
            rc = 0;
        } else {
            fprintf(stderr, "can't find entry for %s\n", argv[1]);
            rc = 1;
        }
        break;

    case 3:
        rc = setpwd(argv[1], argv[2]);
        if (rc != 0) {
            fprintf(stderr, "error setting password: %d\n", rc);
            rc = 1;
            break;
        }
        rc = setspw(argv[1], argv[2]);
        if (rc != 0) {
            fprintf(stderr, "error setting shadow password: %d\n", rc);
            rc = 1;
            break;
        }
        break;

    default:
        fprintf(stderr, "usage: opasswd user [password]\n");
        rc = 1;
    }
    exit(rc);
}
#endif
