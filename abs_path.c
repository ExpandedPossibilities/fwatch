#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>


char**
abs_path (const char *base, const char *rel)
{
  const char *ebase, *erel, *targ, *ip;
  char *out, *op;
  size_t osize;
  int ahead;
  int eat;
  char c;

  ebase = base + strnlen(base, PATH_MAX);
  if(*ebase != 0) {
    errno = ENAMETOOLONG;
    return NULL;
  }

  erel = rel + strnlen(rel, PATH_MAX);
  if(*erel != 0) {
    errno = ENAMETOOLONG;
    return NULL;
  }
  osize = 1 + (erel - rel) + (ebase - base);
  out = malloc(osize);
  if(out == NULL) return NULL; /* preserve errno */
  op = out + osize;
  *op = 0;

  /* -------------------------------*/
  targ = rel;
  for(ip=erel-1; ip >= targ;
      ip = targ == rel && ip == targ? ( targ=base, ebase ) :  (ip-1)){
    c = ip==ebase?'/':*ip;
    printf("%c",c);
  }
  return NULL;
}

int
main (int argc, char** argv)
{
  abs_path(getcwd(NULL,PATH_MAX),argv[1]);
}
