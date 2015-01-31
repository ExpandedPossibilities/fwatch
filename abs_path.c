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
  char c;
  int ahead = 1;
  int eat = 0;


  if(rel == NULL && base == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if(rel == NULL){
    rel = "";
  }

  if(rel[0] == '/') {
    base = NULL;
  }

  if(base == NULL) {
    ebase = NULL;
  } else {
    ebase = base + strnlen(base, PATH_MAX);
    if(*ebase != 0) {
      errno = ENAMETOOLONG;
      return NULL;
    }
  }

  erel = rel + strnlen(rel, PATH_MAX);
  if(*erel != 0) {
    errno = ENAMETOOLONG;
    return NULL;
  }
  osize = 1 + (erel - rel) + (ebase - base);
  out = malloc(osize);
  if(out == NULL) return NULL; /* preserve errno */
  op = out + osize - 1;
  *op = 0;

  /* -------------------------------*/
  eat = 0;
  ahead = 1;

  targ = rel;
  for(ip=erel-1;
      ip >= targ;
      ip = targ == rel && ip == targ && base? ( targ=base, ebase ) :  (ip-1)){
    c = ip==ebase?'/':*ip;
    printf("%c",c);
    if(ahead > 0){
      if(ahead <= 3){
        if(c == '/'){
          if(ahead == 1) {
            continue; /* consume runs of slashes */
          }else if(ahead == 2) {
            ahead = 1; /* ./ */
            continue;
          } else { /* ../ */
            eat++;
            ahead = 1;
            continue;
          }
        } else if (c == '.') {
          ahead++;
          continue;
        }
      }
      while(--ahead > 0){
        if(eat == 0) *(--op)='.';
      }
      if(eat == 0) *(--op) = c;
    } else {
      if(eat == 0) *(--op) = c;
      if(c == '/'){
        if(eat>0) eat--;
        ahead = 1;
      }
    }
  }
  if(op>out){
    printf("\nX:%p\nY:%p",(void*)op, (void*)out);
    osize = out+osize-op;
    memmove(out, op, osize);
    out[osize]=0;
    realloc(out, osize);
  }
  printf("\n>>%s<<\n",out);
  return NULL;
}

int
main (int argc, char** argv)
{
  abs_path(getcwd(NULL,PATH_MAX),argv[1]);
}
