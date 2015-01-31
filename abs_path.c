#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <err.h>


#define ounshift(x)                             \
  do{                                           \
  if(eat == 0) {                                \
    if(--op < out){                             \
      errno = ERANGE;                           \
      return NULL;                              \
    } else {                                    \
      *op=(x);                                  \
    }                                           \
  }} while(0)


#define xounshift(x)  if(eat == 0) *(--op)=(x)

/*
  Given a base path and a relative path, returns a path containing
  no instances of directories named "." or "..".

  Either the base path or the relative path may be omitted.

  If `output' is NULL, a newly allocated buffer of sufficient size
  will be used.

  If `output' is non-NULL, and outlen is smaller than the path to
  be returned, returns NULL and sets errno to ERANGE. The content
  of *output is undefined in this case. Note that since the real
  number of bytes needed is not knowable in advance, substantial
  work may have been done before the size mismatch is discovered.

  Stores number of bytes actually used in `*used' if `used' is
  non-NULL.
 */
char*
abs_path (const char *base, const char *rel,
          char* output, size_t outlen, size_t* used)
{
  const char *ebase, *erel, *targ, *ip;
  char *out, *op;
  size_t oused, maxosize;
  char c;
  int ahead = 1;
  int eat = 0;

  if(rel == NULL && base == NULL) {
    /* at least one of rel or base must be supplied */
    errno = EINVAL;
    return NULL;
  }

  if(rel == NULL){
    /* if only one of (rel, base) are present, put it in rel */
    rel = base;
    base = NULL;
  }

  if(rel[0] == '/') {
    /* ignore base if rel is absolute */
    base = NULL;
  }

  if(base == NULL) {
    /* next line prevents subtraction from breaking in length calc */
    ebase = NULL;
  } else {
    /* establish a reference to the end of base */
    ebase = base + strnlen(base, PATH_MAX);
    if(*ebase != 0) {
      errno = ENAMETOOLONG;
      return NULL;
    }
  }

  /* establish a reference to the end of rel */
  erel = rel + strnlen(rel, PATH_MAX);
  if(*erel != 0) {
    errno = ENAMETOOLONG;
    return NULL;
  }

  /* maximum output is:
     size of rel + 1 for terminating NULL
     plus (if base is non-null) size of base + 1 for slash */
  maxosize = (erel - rel) + 1 + (base==NULL?0:(ebase - base)+1);

  if(output == NULL){
    /* allocate the output buffer */
    out = malloc(maxosize);
    if(out == NULL) return NULL; /* preserve errno */
    op = out + maxosize - 1;
  } else {
    /* use the supplied buffer */
    out = output;
    op = out + MIN(maxosize,outlen) - 1;
  }

  /* ensure output contains final NULL */
  *op = 0;

  /* iterate backwards over the logical concatenation of base, "/",
     and rel, without consuming memory to hold the concatenation.

     targ starts off pointing at rel, switches to base when rel is
     consumed */

  targ = rel;
  for(ip=erel-1;
      ip >= targ;
      ip = targ == rel && ip == targ && base? ( targ=base, ebase ) :  (ip-1)){
    /* ebase, if defined, points at the null byte at the end of base
       the assignment below acts as if that null byte were the slash
       between base and rel.
       If no base is being used, then rel already starts with a slash,
       ebase is NULL, and the conditional never takes the first branch. */
    c = ip==ebase?'/':*ip;
    /* DEBUG: printf("%c",c); */
    /* ahead holds the number of bytes not yet emitted (because they might
       represent "./" or "../" instructions */
    if(ahead > 0){
      if(ahead <= 3){
        /* when ahead <=3 this might still be a path operation,
           othwerise it just indicates a directory whose name
           ends in .. */
        if(c == '/'){
          if(ahead == 1) {
            continue; /* consume runs of slashes */
          }else if(ahead == 2) {
            ahead = 1; /* ./ */
            continue;
          } else { /* ../ */
            /* eat holds the number of directors to omit from output 
               due to the presence of one or more runs of "../" */
            eat++;
            ahead = 1;
            continue;
          }
        } else if (c == '.') {
          /* this supports counting "." and ".." */
          ahead++;
          continue;
        }
      }
      /* Flow arrives here when the bytes not yet emitted are known
         to represent an acual part of the path an not a special
         instruction.
         The next step is to emit the missing bytes, which will be
         a series of "." characters followed by the current character
         under analysis */
      while(--ahead > 0){
        ounshift('.');
      }
      ounshift(c);
    } else {
      /* emit the current byte unless we're still obeying a "../" */
      ounshift(c);
      if(c == '/'){
        if(eat>0) eat--;
        /* next bytes might be part of a path traversal instruction */
        ahead = 1;
      }
    }
  }

  /* calculate memory used, including final NULL byte */
  oused = out+maxosize-op;

  /* Inform caller of length of path as if strlen were called on
     the return value.
     This is intentionally done here, before any errors can occur in
     the next block. */
  if(used) *used = oused-1;

  /* DEBUG: printf ("\n>>%zu\n",oused); */
  if(op>out){
    /* we will have consumed more bytes than allocated in `out'
       IFF "./" or "//" or "../" are present
       In that case, the first few bytes of `out' are garbage.
       Move the string back to out and release the unused memory */
    /* DEBUG: printf("\nA:%p\nB:%p",(void*)op, (void*)out); */
    memmove(out, op, oused);
    /* not needed: out[oused]=0; */
    if(output == NULL) {
      /* the buffer was allocated in this function and can
         be resized */
      op = realloc(out, oused);
      if(op == NULL){
        free(out);
        return NULL; /* preserve errno */
      }
      if(op != out ) {
        free(out);
      }
      out = op;
    }
  }
  return out;
}

int
main (int argc, char** argv)
{
  char* pth = abs_path(getcwd(NULL,PATH_MAX), argv[1], NULL, 0, NULL);
  if(!pth) err(1,"not sure what happened!");
  printf("%s", pth );
  return 0;
}
