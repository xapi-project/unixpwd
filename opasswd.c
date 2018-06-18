
/*
 * A first shot at replacing 
 * https://github.com/xapi-project/ocaml-opasswd.git
 * with a C library and a minimal interface to OCaml.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <shadow.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>

/* getpw: set pw to the encrypted password for user and return 0 on
 * success. The memory pointed to by pw must be freed by the caller.
 *
 * On failure, no memory is allocated and errno != 0 is returned.
 */
static int
getpw(char *user, char **pw)
{

  if (access(SHADOW, R_OK) == 0) {
    struct spwd *spw;
    spw = getspnam(user);
    if (!spw) {
      *pw = NULL;
      return errno;
    }
    *pw = strdup(spw->sp_pwdp);
  } else {
    struct passwd *pwd;
    pwd = getpwnam(user);
    if (!pwd) {
      *pw = NULL;
      return errno;
    }
    *pw = strdup(pwd->pw_passwd);
  }
  return 0;
}

#define ETC_PASSWD "passwd"
#define TMP_PASSWD "passwd.tmp"

/* setpwd
 * set password entry for a user
 */

static int
setpwd(char *user, char* cipher)
{

  struct passwd *pwd;
  FILE *tmp;

  umask(0033);
  tmp = fopen(TMP_PASSWD, "w");
  if (!tmp) {
    return errno;
  } 
  
  setpwent();
  while (pwd = getpwent()) {
    if(!strcmp(user, pwd->pw_name)) {
      char *old = pwd->pw_passwd;
      pwd->pw_passwd = cipher;
      putpwent(pwd, tmp);
      pwd->pw_passwd = old;
    } else {
      putpwent(pwd, tmp);
    }
  }
  endpwent();

  fclose(tmp);
  if (!rename(TMP_PASSWD,ETC_PASSWD)) {
    return errno;
  }
  
  return 0;
}

#define ETC_SPASSWD "shadow"
#define TMP_SPASSWD "shadow.tmp"

/* setspw
 * set a shadow password for a user
 */
static int
setspw(char *user, char* cipher)
{

  struct spwd *pwd;
  FILE *tmp;

  umask(0037);
  tmp = fopen(TMP_SPASSWD, "w");
  if (!tmp) {
    return errno;
  } 
  
  setspent();
  while (pwd = getspent()) {
    if(!strcmp(user, pwd->sp_namp)) {
      char *old = pwd->sp_pwdp;
      pwd->sp_pwdp = cipher;
      putspent(pwd, tmp);
      pwd->sp_pwdp = old;
    } else {
      putspent(pwd, tmp);
    }
  }
  endspent();

  fclose(tmp);
  if (!rename(TMP_SPASSWD,ETC_SPASSWD)) {
    return errno;
  }
  
  return 0;
}

/* unshadow
 * return /etc/passwd as a string but with password entries from the
 * shadow password file if it has a corresponding entry.
 * The memory returned by this function must be freed by the caller.
 */
char *
unshadow(void)
{
  struct spwd *spw;
  struct passwd *pwd;
  char *buf, *cur, *end;
  int bufsize = 64 * 1024;

  buf = malloc(bufsize);
  if (!buf) {
    return NULL;
  }
  cur = buf;
  end = buf + bufsize;

  setpwent();
  while (pwd = getpwent()) {
    spw = getspnam(pwd->pw_name);
    cur += snprintf(cur, end-cur,"%s:%s:%d:%d:%s:%s\n",
        pwd->pw_name,
        spw ? spw->sp_pwdp : pwd->pw_passwd,
        pwd->pw_uid,
        pwd->pw_gid,
        pwd->pw_gecos,
        pwd->pw_dir,
        pwd->pw_shell);
  }
  endpwent();
  if (cur >= end) {
    free(buf);
    buf = NULL;
  }
  return buf;
}

int main(int argc, char**argv)
{
  int rc;
  char *pw;
  char *buf;

  switch (argc) {
    case 1:
      buf = unshadow();
      if (buf) {
        puts(buf);
        free(buf);
      } else {
        fprintf(stderr,"can't unshadow\n");
      }
      break;

    case 2: 
      if (getpw(argv[1],&pw) == 0) {
        printf("%s: %s\n", argv[1], pw);
        free(pw);
        rc = 0;
      } else {
        fprintf(stderr,"can't find entry for %s\n", argv[1]);
        rc = 1;
      }
      break;
            
    case 3:
      rc = setspw(argv[1],argv[2]);
      if (rc != 0) {
        fprintf(stderr,"error setting password: %d\n", rc);
        rc = 1;
      }
      break;

    default:
      fprintf(stderr,"usage: opasswd user [password]\n");
      rc = 1;
  }
  exit(rc);
}


