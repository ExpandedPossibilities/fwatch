/*
 * Copyright (c) 2015, Expanded Possibilities, Inc.
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

#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <err.h>

#include "canonicalpath.h"
#include "splint_defs.h"

#ifndef CP_DEBUG
#define CP_DEBUG 0
#endif

#ifdef S_SPLINT_S
static const char* CP_EMPTY_PATH = "";
#define report_error perror
#define debug_print printf
#define debug_printf printf
#else
#define CP_EMPTY_PATH ""
#define debug_print(str)        debug_printf("%s", str)
#define debug_printf(fmt, ...)  do { if(CP_DEBUG)       \
      fprintf(stderr, fmt, __VA_ARGS__); } while(0)
#endif

#define ounshift(x)                             \
  do{                                           \
  if(eat == 0){                                 \
    if(--op < out){                             \
      errno = ERANGE;                           \
      goto ERR;                                 \
    } else {                                    \
      *op=(x);                                  \
    }                                           \
  }} while(0)


/*@null@*/
char *
canonicalpath(/*@null@*/ const char *base,
              /*@null@*/ const char *rel,
              /*@null@*/ /*@returned@*/ char *output,
              size_t outputsize,
              /*@null@*/ size_t *used)
{
  const char *ebase, *erel, *targ, *ip;
  char *out = NULL;
  char *op;
  size_t oused, maxosize, osize, slen;
  char c;
  int ahead = 1;
  int eat = 0;

  /*@owned@*/ char *tofree = NULL;

  if(output != NULL && outputsize < 1){
    errno = ERANGE;
    goto ERR;
  }

  /* idea */

  if(rel != NULL && *rel == '/'){
    /* rel is absolute, ignore base */
    base = NULL;
  } else if(base == NULL) {
    /* rel is not absolute, but base is null. put cwd in base */

/*@-nullpass@*/
    tofree = getcwd(NULL, 0);
/*@=nullpass@*/

    if(tofree == NULL){
      goto ERR;
    }
    base = tofree;
  }

  /* initial slash is added at the end of the loop */

  while(base != NULL && *base == '/') base++;
  if(base != NULL && *base == '\0') base = NULL;

  while(rel != NULL && *rel == '/') rel++;
  if(rel != NULL && *rel == '\0') rel = NULL;

  /* if only one of (rel, base) are present, put it in rel */
  if(rel == NULL) {
    rel = base;
    base = NULL;
  }

  /* /idea */

  maxosize = 2; /* room for '/' and '\0' */

  /* establish a reference to the end of base */
  if(base == NULL) {
    ebase = NULL;
  } else {
    slen = strnlen(base, PATH_MAX);
    ebase = base + slen;
    if(*ebase != '\0'){
      errno = ENAMETOOLONG;
      goto ERR;
    }
    maxosize += slen + 1;
  }

  if(rel == NULL) rel = CP_EMPTY_PATH;

  /* establish a reference to the end of rel */
  slen = strnlen(rel, PATH_MAX);
  erel = rel + slen;
  if(*erel != '\0'){
    errno = ENAMETOOLONG;
    goto ERR;
  }
  maxosize += slen;

  if(output == NULL){
    /* allocate the output buffer */
    osize = maxosize;
    out = malloc(osize);
    if(out == NULL){
      goto ERR;
    }
  } else {
    /* use the supplied buffer */
    out = output;
    osize = MIN(maxosize, outputsize);
  }

  op = out + osize - 1;

  /* ensure output contains final NULL */
  *op = '\0';

  /*
   * iterate backwards over the logical concatenation of base, "/", and rel,
   * without consuming memory to hold the concatenation.
   *
   * targ starts off pointing at rel, switches to base when rel is consumed
   */

  targ = rel;
  for(ip = erel - 1;
      ip >= targ;
      ip = (targ == rel && ip == targ && base != NULL ?
            (targ = base, ebase) :  (ip - 1))){
    /*
     * ebase, if defined, points at the null byte at the end of base
     * the assignment below acts as if that null byte were the slash
     * between base and rel.
     * If no base is being used, then rel already starts with a slash,
     * ebase is NULL, and the conditional never takes the first branch.
     */
/*@-nullderef@*/
    c = ip == ebase ? '/' : *ip; /* ip cannot be null, splint is wrong */
/*@=nullderef@*/
    debug_printf("%c", c);

    /*
     * ahead holds the number of bytes not yet emitted (because they might
     * represent "./" or "../" instructions
     */
    if(ahead > 0){
      if(ahead <= 3){
        /*
         * when ahead <=3 this might still be a path operation,
         * otherwise it just indicates a directory whose name
         * ends in ..
         */
        if(c == '/'){
          if(ahead == 1){
            continue; /* consume runs of slashes */
          } else if(ahead == 2){
            ahead = 1; /* ./ */
            continue;
          } else { /* ../ */
            /*
             * eat holds the number of directors to omit from output
             * due to the presence of one or more runs of "../"
             */
            eat++;
            ahead = 1;
            continue;
          }
        } else if(c == '.'){
          /* this supports counting "." and ".." */
          ahead++;
          continue;
        }
      }
      /*
       * Flow arrives here when the bytes not yet emitted are known to
       * represent an actual part of the path an not a special
       * instruction.  The next step is to emit the missing bytes,
       * which will be a series of "." characters followed by the
       * current character under analysis
       */
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
  if(*op != '/')  ounshift('/');

  /* calculate memory used, including final NULL byte */
  oused = (size_t) (out + osize - op);

  debug_printf("\n>>%zu\n", oused);
  if(op > out){
#ifdef CP_TRACK_OVERSIZE
    cp_did_oversize = 1;
#endif
    /*
     * we will have consumed more bytes than a in `out' IFF
     * "./" or "//" or "../" are present In that case, the first few
     * bytes of `out' are garbage.  Move the string back to out and
     * release the unused memory
     */

    debug_printf("\nA:%p\nB:%p\n\n", (void*)op, (void*)out);

    if(oused == 1){
      /*
       * op is always > out in this block, so oused is always at least 1.
       * An oused of 1 indicates that the string to emit is "\0"
       * This happens when more ..'s exist than parent directories.
       * Instead, we will emit "/\0"
       */
      if(osize < 2){
        /* requires enough space to return "/\0" */
        errno = ERANGE;
        goto ERR;
      }
      oused = 2;
      op = out;
      op[0] = '/';
      op[1] = '\0';
    } else {
      memmove(out, op, oused);
    }

    if(out != output){
      /* The buffer was allocated in this function and can be resized */
/*@-temptrans@*/
      /*
       * splint thinks `out' aliases `output', but the conditional disagrees.
       */
      op = realloc(out, oused);
/*@=temptrans@*/
      if(op == NULL){
        goto ERR;
      }
      out = op;
    }
  } else {
#ifdef CP_TRACK_OVERSIZE
    cp_did_oversize = 0;
#endif
  }

  /*
   * Inform caller of length of path as if strlen were called on
   * the return value.
   */
  if(used) *used = oused - 1;

  free(tofree);
  return out;

ERR:
/*@-usereleased -temptrans@*/
  /* splint fails to notice the return above and calls this a double-free */
  free(tofree);
  /* splint thinks `out' aliases `output', but the conditional disagrees. */
  if(output != out)  free(out);
/*@-temptrans =usereleased@*/

  /* NOTE: errno has been set by canonicalpath or a library function */
  return NULL;
}
