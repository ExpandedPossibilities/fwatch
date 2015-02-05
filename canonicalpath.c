/*
 * Copyright (c) 2015, Eric Kobrin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 *  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 *  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <errno.h>
#include <err.h>
#include "canonicalpath.h"

#define ounshift(x)                             \
  do{                                           \
  if(eat == 0) {                                \
    if(--op < out){                             \
      if(output==NULL) free(out);               \
      free(tofree);                             \
      errno = ERANGE;                           \
      return NULL;                              \
    } else {                                    \
      *op=(x);                                  \
    }                                           \
  }} while(0)

/* Convenience method for common use case. */

/*@null@*/
char *
canpath(/*@null@*/ const char *base, /*@null@*/ const char *rel)
{
  return canonicalpath(base, rel, NULL, 0, NULL);
}

/*
  Given a base path and a relative path, returns a path containing
  no instances of directories named "." or "..".

  When the base path is omitted, canonicalpath will behave as if
  the base path had been populated with getcwd(NULL, 0).

  Both the base path or the relative path may be omitted (passed NULL).

  If more "../" elements are encountered than there are preceding
  directories, in the path, extra "../" elements are silently consumed.
  This behavior is similar to calling "cd .." from the root of the
  file system.

  If `output' is NULL, a newly allocated buffer of sufficient size
  will be used. This buffer may later be free(3)'d.

  If `output' is non-NULL, and outlen is smaller than the path to
  be returned, returns NULL and sets errno to ERANGE. The content
  of *output is undefined in this case. Note that since the real
  number of bytes needed is not knowable in advance, substantial
  work may have been done before the size mismatch is discovered.

  Stores number of bytes actually used in `*used' if `used' is
  non-NULL.

  RETURN VALUES
  -------------

  If successfull, returns a pointer to a canonicalized version of
  the `rel' relative to `base' (or to the current working directory
  if `base' is NULL. If any error occurs, returns NULL and ensures
  that a useful value is present in errno.


  ERRORS
  ------

  [ENAMETOOLONG]    The `base' or `rel' parameters contain no NUL
                    bytes within PATH_MAX bytes.

  [ERANGE]          The `output' parameter was supplied and the
                    canonical path is longer than `outlen'

  canonicalpath may additionally return NULL and set errno to any
  of the values specified by the library functions malloc(3),
  realloc(3), or getcwd(3).
 */

/*@null@*/
char *
canonicalpath(/*@null@*/ const char *base,
              /*@null@*/ const char *rel,
              /*@null@*/ char *output,
              size_t outlen,
              /*@null@*/ size_t *used)
{
  const char *ebase, *erel, *targ, *ip;
  char *out, *op;
  size_t oused, maxosize;
  char c;
  int ahead = 1;
  int eat = 0;
  char *tofree = NULL;

  if(base == NULL) {
    tofree = getcwd(NULL, 0);
    if(tofree == NULL) {
      return NULL; /* preserve errno */
    }
    base = tofree;
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
    if(*ebase != '\0') {
      free(tofree);
      errno = ENAMETOOLONG;
      return NULL;
    }
  }

  /* establish a reference to the end of rel */
  erel = rel + strnlen(rel, PATH_MAX);
  if(*erel != '\0') {
    free(tofree);
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
    if(out == NULL) {
      free(tofree);
      return NULL; /* preserve errno */
    }
    op = out + maxosize - 1;
  } else {
    /* use the supplied buffer */
    out = output;
    op = out + MIN(maxosize, outlen) - 1;
  }

  /* ensure output contains final NULL */
  *op = '\0';

  /* iterate backwards over the logical concatenation of base, "/", and rel,
     without consuming memory to hold the concatenation.

     targ starts off pointing at rel, switches to base when rel is consumed */

  targ = rel;
  for(ip = erel - 1;
      ip >= targ;
      ip = targ == rel && ip == targ && base != NULL ?
        (targ=base, ebase) :  (ip-1)){
    /* ebase, if defined, points at the null byte at the end of base
       the assignment below acts as if that null byte were the slash
       between base and rel.
       If no base is being used, then rel already starts with a slash,
       ebase is NULL, and the conditional never takes the first branch. */
    c = ip == ebase ? '/' : *ip;
    /* DEBUG: printf("%c",c); */
    /* ahead holds the number of bytes not yet emitted (because they might
       represent "./" or "../" instructions */
    if(ahead > 0){
      if(ahead <= 3){
        /* when ahead <=3 this might still be a path operation,
           othwerise it just indicates a directory whose name
           ends in .. */
        if(c == '/'){
          if(ahead == 1){
            continue; /* consume runs of slashes */
          } else if(ahead == 2){
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
        if(eat > 0) eat--;
        /* next bytes might be part of a path traversal instruction */
        ahead = 1;
      }
    }
  }

  /* calculate memory used, including final NULL byte */
  oused = out + maxosize - op;

  /* Inform caller of length of path as if strlen were called on
     the return value.
     This is intentionally done here, before any errors can occur in
     the next block. */
  if(used) *used = oused - 1;

  /* DEBUG: printf ("\n>>%zu\n",oused); */
  if(op > out){
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
        free(tofree);
        free(out);
        return NULL; /* preserve errno */
      }
      out = op;
    }
  }
  free(tofree);
  return out;
}
