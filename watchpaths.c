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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include "watchpaths.h"
#include "canonicalpath.h"
#include "reallocarray.h"

#ifndef WP_DEBUG
#define WP_DEBUG 0
#endif

#ifndef WP_COMPLAIN
#define WP_COMPLAIN 0
#endif

#ifdef S_SPLINT_S
#define report_error perror
#define debug_print printf
#define debug_printf printf
#else
#define report_error(x)         do { if(WP_COMPLAIN) perror(x); } while(0)
#define debug_print(str)        debug_printf("%s",str)
#define debug_printf(fmt, ...)  do { if(WP_DEBUG)       \
      fprintf(stderr, fmt, __VA_ARGS__); } while(0)
#endif


/*
 * struct pathinfo
 *
 * holds the per-path state for watchpaths, allowing callers to watch
 * multiple paths simultaneously
 */
struct pathinfo {
  dev_t dev;
  /*@owned@*/ char *path;
  /*@owned@*/ char **slashes;
  /*@dependent@*/ char **nextslash;
  /*@dependent@*/ char **endslash;
  /*@dependent@*/ struct kevent *ke;
  int index;
  /*@dependent@*/ long *fdp;
};


/*@null@*/
static char **find_slashes(char *path, int len, /*@out@*/ size_t *sout);

static int    walk_to_extant_parent(struct pathinfo *pinfo);

/*@null@*/
static char **
find_slashes(char *path, int len, /*@out@*/ size_t *sout)
{
  size_t max = 0;
  size_t i = 0;
  size_t count = 1;
  char *p = path;
  char **out = NULL;

  max = len >= 0 ? (size_t) len : PATH_MAX;
  while(i < max && *p != '\0'){
    if(*(p++) == '/'){
      count++;
    }
  }

  out = reallocarray(NULL, count, sizeof(char *));
  if(out == NULL){
    *sout = 0;
    return NULL; /* keeps errno */
  }

  p = path;

  out[count - 1] = p; /* count is always at least 1 */
  for(i = count - 2; i > 0; i--){
    do { p++; } while(*p != '/');
    out[i] = p;
  }
  out[0] = NULL; /* hack to detect slash record for leaf */
  *sout = count;
  return out;
}

static int
walk_to_extant_parent(struct pathinfo *pinfo)
{
  struct stat finfo;

  pinfo->nextslash = pinfo->slashes;
  if(*pinfo->fdp >= 0){
    (void) close((int)*pinfo->fdp);
  }
  for(*pinfo->fdp = (long) open(pinfo->path, O_RDONLY);
      /* open(2) errors checked in caller */
      *pinfo->fdp == -1 &&
      (errno == ENOENT ||
       errno == ENOTDIR ||
       errno == EACCES ||
       errno == EPERM) && pinfo->nextslash < pinfo->endslash;
      pinfo->nextslash++) {
    if(*pinfo->nextslash) **pinfo->nextslash = '\0';
    debug_printf("open %s: ", pinfo->path);
    *pinfo->fdp = (long) open(pinfo->path, O_RDONLY);
    debug_printf("%ld\n", *pinfo->fdp);
    if(*pinfo->nextslash) **pinfo->nextslash = '/';
    if(*pinfo->fdp >= 0){
      if(-1 == fstat((int) *pinfo->fdp, &finfo)){
        /* let caller see errno */
        return -1;
      }
      if(pinfo->dev == -1){
        /* bootstrap initial device choice */
        pinfo->dev = finfo.st_dev;
      } else if(finfo.st_dev != pinfo->dev){
	errno = EXDEV; /* bad cross-device traversal */
        return -1;
      }
    }
  }
  if(pinfo->nextslash > pinfo->slashes){
    pinfo->nextslash--;
  }
  return 0;
}


int
watchpaths(char **inpaths, int numpaths,
           void (*callback) (u_int, int, void *, int *), void *blob)
{
  /*@owned@*/ struct kevent *kes = NULL;
  int kq = -1;
  /*@owned@*/ struct kevent *eventbuff = NULL;
  /*@dependent@*/ struct kevent *evt = NULL;
  int count = 0;
  int i = 0;
  int cont = 1;
  int ret = -1;
  size_t numslashes = 0;
  /*@owned@*/ char *basepath = NULL;
  /*@owned@*/ struct pathinfo *pinfos = NULL;
  /*@dependent@*/ struct pathinfo *pinfo = NULL;
  u_int typemask = 0;
  u_int types[] = {NOTE_DELETE,
                   NOTE_WRITE,
                   NOTE_EXTEND,
#ifdef NOTE_TRUNCATE
                   NOTE_TRUNCATE,
#endif
                   NOTE_RENAME};
  char *type_names[] = {"Delete", "Write", "Extend", "Truncate", "Rename"};
  int numtypes = 0;

  numtypes = (int) (sizeof(types)/sizeof(types[0]));
  for(i = 0; i < numtypes; i++){
   typemask |= types[i];
  }

  kq = kqueue();
  if(kq == -1){
    report_error("Unable to create queue");
    goto ERR;
  }

  pinfos = reallocarray(NULL, numpaths, sizeof(struct pathinfo));
  if(pinfos == NULL){
    report_error("Unable to allocate path info storage");
    goto ERR;
  }

  kes = reallocarray(NULL, numpaths, sizeof(struct kevent));
  if(kes == NULL){
    report_error("Unable to allocate event setup storage");
    goto ERR;
  }

 /* Followint "+ 1" is to include space for an error event per kevent(2) */
  eventbuff = reallocarray(NULL, numpaths + 1, sizeof(struct kevent));
  if(eventbuff == NULL){
    report_error("Unable to allocate event storage");
    goto ERR;
  }

  for(i = 0; i < numpaths; i++){
    pinfos[i].index = i;
    pinfos[i].path = NULL;
    if(!inpaths[i]){
      errno = EINVAL;
      report_error("NULL pathname provided to watchpaths");
      goto ERR;
    }
    if(inpaths[i][0] == '/'){
      pinfos[i].path = strndup(inpaths[i], PATH_MAX);
      if(pinfos[i].path == NULL){
        report_error("Unable to allocate space for path of file to watch");
        goto ERR;
      }
    } else {
      if(basepath == NULL){
/*@-nullpass@*/
        basepath = getcwd(NULL, 0);
/*@=nullpass@*/
        if(basepath == NULL){
          report_error("Unable to find current path, needed for watching"
                       " relaive paths");
          goto ERR;
        }
      }
      pinfos[i].path = canonicalpath(basepath, inpaths[i], NULL, 0, NULL);
      if(pinfos[i].path == NULL){
        report_error("Unable to find path for file to watch");
        goto ERR;
      }
    }

    debug_printf("Watching for %s\n", pinfos[i].path);
    pinfos[i].slashes = find_slashes(pinfos[i].path, -1, &numslashes);
    if(pinfos[i].slashes == NULL){
      report_error("Unable to allocate space to track slashes in pathname");
      goto ERR;
    }
    pinfos[i].endslash = pinfos[i].slashes + numslashes;
    pinfos[i].nextslash = pinfos[i].slashes;
    pinfos[i].dev = -1;
    EV_SET(&kes[i], -1, EVFILT_VNODE, EV_ADD | EV_ONESHOT, typemask,
           0, &pinfos[i]);
    pinfos[i].ke = &kes[i];
    pinfos[i].fdp = (long *)&(kes[i].ident);

    if(walk_to_extant_parent(&pinfos[i]) == -1){
       report_error("unable to do parent walk");
       goto ERR;
    }

    if(*pinfos[i].fdp == -1){
      report_error("unable to open file for watching");
      goto ERR;
    }
  }

  while(cont != 0){
    evt = eventbuff;
    /* TODO: support timespec from caller */
    count = kevent(kq, kes, numpaths, evt, numpaths + 1, NULL);

    if(count > 0){
      for(; evt < &eventbuff[count]; evt++){
        if(evt->flags & EV_ERROR){
          errno = evt->data;
          report_error("error in event list");
          /* exit to stop loops */
          goto ERR;
        } else {
          pinfo = evt->udata;

          if(*pinfo->nextslash) **pinfo->nextslash = '\0';
          debug_printf("EVT: %s\n", pinfo->path);
          if(*pinfo->nextslash) **pinfo->nextslash = '/';

          for(i = 0; i < numtypes; i++){
            if(evt->fflags & types[i]){
                    debug_printf("--Matched: %s\n", type_names[i]);
            }
          }
          /* NOTE_DELETE might be a new file copied onto the old path,
             needs work to follow. */
          if(evt->fflags & (NOTE_DELETE | NOTE_RENAME)){
            pinfo->nextslash++;
            if(pinfo->nextslash >= pinfo->endslash){
              debug_print("Parents deleted to root of device. Giving up.\n");
              goto ERR;
            }
          }

          if(*pinfo->nextslash){
            /* *pinfo->nextslash == 0 when examining leaf */
            if(walk_to_extant_parent(pinfo) == -1){
               report_error("unable to do parent walk");
               goto ERR;
            }
            if(*pinfo->fdp == -1){
              report_error("Unrecoverable error encountered trying "
                           "to open a file");
              goto ERR;
            }
          }

          if(pinfo->nextslash == pinfo->slashes){
/*@-noeffect@*/
            callback(evt->fflags, pinfo->index, blob, &cont);
/*@=noeffect@*/
          }
        }
      }
    } else {
      report_error("error calling kevent");
      /* exit to stop loops */
      goto ERR;
    }
  }

  ret = 0;

ERR:
  if(pinfos){
    for(i = 0; i < numpaths; i++){
      free(pinfos[i].slashes);
      free(pinfos[i].path);
    }
  }
  free(basepath);
  free(pinfos);
  free(kes);
  free(eventbuff);
  return ret;
}





