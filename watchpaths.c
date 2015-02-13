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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
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

#ifdef O_EVTONLY
#define OPEN_MODE O_EVTONLY
#else
#define OPEN_MODE O_RDONLY
#endif

/*
 * struct pathinfo
 *
 * This holds the per-path state for watchpaths, allowing callers to
 * watch multiple paths simultaneously. These variables were extracted
 * from watchpath (single path version) when converting to the current
 * version which simultaneously watches multiple paths.
 *
 * dev:        the device id of the leaf node
 * path:       the path to be watched
 * slashes:    an array of pointers to the '/' characters in `path'
 * nextslash:  a pointer to the next element of `slashes' to examine
 * endslash:   a pointer to the final element of `slashes'
 * ke:         a reference to the kevent structure passed to kevent(2)
 *             for this path. It points to an element of the changelist
 *             array.
 * index:      the index in the array of paths to watch which corresponds
 *             to this structure.
 * fdp:        a pointer used to extract the file descriptor from a kevent
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


/* find_slashes
 *
 * Returns an array of pointers to the slashes in the path argument.
 *
 * The caller sets the value at one of the pointers to '\0' in order
 * to "truncate" the path string at a particular parent directory.
 *
 * Common usage:
 *  if(*nextslash) **nextslash = '\0';
 *  CODE THAT USES SHORTENED PATH
 *  if(*nextslash) **nextslash = '/';
 *
 * The reason for `if(*nextslash)' is that the 0th "slash" pointer
 * returned is set to NULL, so that the caller's slash-walking loop
 * can distinguish between the leaf and one of its parent directories.
 *
 * ARGUMENTS
 *
 * path: the string to search
 * len:  the length of the path string or zero to calculate the length
 * sout: a pointer to a size_t which will be populated with the length
 *       of the return array
 */

/*@null@*/
static char **
find_slashes(char *path, int len, /*@out@*/ size_t *sout)
{
  size_t max = 0;
  size_t i = 0;
  size_t count = 1;
  char *p = path;
  char **out = NULL;

  /* count the number of slashes */
  max = len > 0 ? (size_t) len : PATH_MAX;
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
  /* count is always at least 1 */
  for(i = count - 1; i > 0; i--){
    while(*p != '/') {
      p++;
    }
    out[i] = p++;
  }
  out[0] = NULL; /* hack to detect slash record for leaf */
  *sout = count;
  return out;
}

/*
 * walk_to_extant_parent
 *
 * Given a pointer to a pinfo struct, modifies the structure to
 * reference the first parent directory of pinfo->path which actually
 * exists.
 *
 * Stops at device boundaries.
 *
 * Returns 0 if successful, returns -1 and sets errno otherwise.
 */
static int
walk_to_extant_parent(struct pathinfo *pinfo)
{
  struct stat finfo;

  /*
   * start at the leaf every time in case multiple path elements were
   * created at once, such as by mv
   */
  pinfo->nextslash = pinfo->slashes;
  if(*pinfo->fdp >= 0){
    /* don't leak file descriptors */
    while(-1 == close((int)*pinfo->fdp) && errno == EINTR);
  }

  /*
   * loop over each parent directory until one is encountered that
   * exists or returns an error other than one of the ones that may go
   * away when the missing directory is recreated
   */
  for(*pinfo->fdp = (long) open(pinfo->path, OPEN_MODE);
      /* open(2) errors checked in caller */
      *pinfo->fdp == -1 &&
      (errno == ENOENT ||
       errno == ENOTDIR ||
       errno == EACCES ||
       errno == EPERM ||
       errno == EINTR) && pinfo->nextslash < pinfo->endslash;
      pinfo->nextslash++){
    if(errno == EINTR) continue;

    /* temporarily truncate pinfo->path at the next slash */
    if(*pinfo->nextslash) **pinfo->nextslash = '\0';
    debug_printf("open %s: ", pinfo->path);
    *pinfo->fdp = (long) open(pinfo->path, OPEN_MODE);
    debug_printf("%ld\n", *pinfo->fdp);
    /* restore the slash in pinfo->path */
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

/*
 * Caller documentation is in watchpaths.h
 * Implementation discussion follows.
 *
 * Watchpaths attempts to monitor for changes in any one of a list of
 * specified files. It uses the kqueue/kevent API to do so. This API
 * does not have an option for monitoring path names, but does allow
 * monitoring file descriptors.
 *
 * Several design considerations follow from this fact:
 *
 * 1. watchpaths() only functions properly if it is able to call
 *    open(2) on the file. On systems where O_EVTONLY mode is
 *    supported that open mode is used. Otherwise, O_RDONLY is used.
 *
 * 2. If files are renamed, such as by a "safe write" operation, work
 *    is required to obtain the file descriptor of the file now
 *    existing at the path.
 *
 * 3. If a file is deleted, there is no mechanism in kqueue to watch
 *    for its recreation. watchpaths() therefore begins watching its
 *    parent directory for writes. When such a write is detected, the
 *    function attempts to open the missing file again. This pattern
 *    is repeated for each parent directory via the
 *    walk_to_extant_parent() function defined above.
 *
 * In order to conserve memory, path walking is achieved not by
 * maintaining multiple copies of prefixes of the target path, but
 * rather by replacing '/' characters in the target path with NUL
 * bytes as needed. All paths are copied by watchpaths, so callers
 * need not worry hat these changes will alter data in the caller's
 * view.
 */
int
watchpaths(char **inpaths, int numpaths,
           void (*callback) (u_int, int, void *, int *), void *blob)
{
  /*@owned@*/ struct kevent *changelist = NULL;
  int kq = -1;
  /*@owned@*/ struct kevent *eventbuff = NULL;
  /*@dependent@*/ struct kevent *evt = NULL;
  int eventcount = 0;
  int i = 0;
  int cont = 1; /* &cont is passed to callback, if set to 0, main loop ends */
  int ret = -1; /* stores return value for watchpaths */
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
  char *type_names[] = {"Delete",
                        "Write",
                        "Extend",
#ifdef NOTE_TRUNCATE
                        "Truncate",
#endif
                        "Rename"};
  int numtypes = 0;

  /* calculate mask to use in EV_SET call */
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

  changelist = reallocarray(NULL, numpaths, sizeof(struct kevent));
  if(changelist == NULL){
    report_error("Unable to allocate event setup storage");
    goto ERR;
  }

  /* Following "+ 1" is to include space for an error event per kevent(2) */
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
      /* absolute paths are used literally without canonicalization */
      /* TODO: consider canonicalizing all paths */
      pinfos[i].path = strndup(inpaths[i], PATH_MAX);
      if(pinfos[i].path == NULL){
        report_error("Unable to allocate space for path of file to watch");
        goto ERR;
      }
    } else {
      /*
       * basepath is stores the current directory, it is only
       * calculated once
       */
      if(basepath == NULL){
/*@-nullpass@*/
        basepath = getcwd(NULL, 0);
/*@=nullpass@*/
        if(basepath == NULL){
          report_error("Unable to find current path, needed for watching"
                       " relative paths");
          goto ERR;
        }
      }
      pinfos[i].path = canonicalpath(basepath, inpaths[i], NULL, 0, NULL);
      if(pinfos[i].path == NULL){
        report_error("Unable to find path for file to watch");
        goto ERR;
      }
    }

    /* TODO: consider emitting the list of watched paths */
    debug_printf("Watching for %s\n", pinfos[i].path);
    pinfos[i].slashes = find_slashes(pinfos[i].path, 0, &numslashes);
    if(pinfos[i].slashes == NULL){
      report_error("Unable to allocate space to track slashes in pathname");
      goto ERR;
    }
    pinfos[i].endslash = pinfos[i].slashes + numslashes;
    pinfos[i].nextslash = pinfos[i].slashes;
    pinfos[i].dev = -1;
    EV_SET(&changelist[i], -1, EVFILT_VNODE, EV_ADD | EV_ONESHOT, typemask,
           0, &pinfos[i]);
    pinfos[i].ke = &changelist[i];
    pinfos[i].fdp = (long *)&(changelist[i].ident);

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
    eventcount = kevent(kq, changelist, numpaths, evt, numpaths + 1, NULL);

    if(eventcount > 0){
      for(; evt < &eventbuff[eventcount]; evt++){
        if(evt->flags & EV_ERROR){
          errno = evt->data;
          report_error("error in event list");
          /* exit to stop loops */
          goto ERR;
        } else {
          pinfo = evt->udata;

          if(WP_DEBUG){
            /* temporarily truncate pinfo->path at the next slash */
            if(*pinfo->nextslash) **pinfo->nextslash = '\0';
            debug_printf("EVT: %s\n", pinfo->path);
            /* restore the slash in pinfo->path */
            if(*pinfo->nextslash) **pinfo->nextslash = '/';

            for(i = 0; i < numtypes; i++){
              if(evt->fflags & types[i]){
                debug_printf("--Matched: %s\n", type_names[i]);
              }
            }
          }

          /*
           * NOTE_DELETE might be a new file copied onto the old path.
           */
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
            /* A watched path was modified. Execute the callback. */
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
  free(changelist);
  free(eventbuff);
  return ret;
}
