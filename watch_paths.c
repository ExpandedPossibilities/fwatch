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
#include "watch_paths.h"
#include "canonicalpath.h"

#define BPOINT do{}while(0)
#define xfree(p) do{if(NULL != (p)){free(p);(p)=NULL;}}while(0)

#ifdef DEBUG_WATCH_PATHS
#define debug_printf printf
#define report_error perror
#else
#define debug_printf(...) //(void)
#define report_error(...) //(void)
#endif

/*! struct pathinfo
 *  holds the per-path state for watch_paths, allowing callers to watch multiple
 *  paths simultaneously
 */
struct pathinfo
{
  dev_t dev;
  char *path;
  char **slashes;
  char **nextslash;
  char **endslash;
  struct kevent *ke;
  int index;
  long *fdp;
};


char**   find_slashes(char* path, int len, int *sout);
int      walk_to_extant_parent(struct pathinfo *pinfo);

char**
find_slashes(char* path, int len, int *sout)
{
  int max = 0;
  int i = 0;
  int count = 1;
  char *p = path;
  char **out = NULL;
  
  max = len >=0 ? len : PATH_MAX;
  while(i<max && *p != 0){
    if(*(p++)=='/'){
      count++;
    }
  }

  out = reallocarray(NULL, count, sizeof(char*));
  if(out == NULL){
    return NULL; /* keeps errno */
  }

  p = path;

  out[count-1] = p; /* count is always at least 1 */
  for(i=count-2; i>0; i--){
    do { p++; } while(*p != '/');
    out[i] = p;
  }
  out[0] = NULL; /* hack to detect slash record for leaf */
  *sout = count;
  return out;
}

int
walk_to_extant_parent(struct pathinfo *pinfo)
{
  struct stat finfo;

  pinfo->nextslash = pinfo->slashes;
  if(*pinfo->fdp >= 0){
    close(*pinfo->fdp);
  }
  for(*pinfo->fdp = open(pinfo->path, O_RDONLY); /* open(2) errors checked in caller */
      *pinfo->fdp == -1 &&
      (errno == ENOENT ||
       errno == ENOTDIR ||
       errno == EACCES ||
       errno == EPERM) && pinfo->nextslash < pinfo->endslash;
      pinfo->nextslash ++) {
    if(*pinfo->nextslash) **pinfo->nextslash = 0;
    debug_printf("open %s: ", pinfo->path);
    *pinfo->fdp = open(pinfo->path, O_RDONLY);
    debug_printf("%ld\n", *pinfo->fdp);
    if(*pinfo->nextslash) **pinfo->nextslash = '/';
    if(*pinfo->fdp >= 0){
      if(-1 == fstat(*pinfo->fdp, &finfo)){
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
watch_paths(char **inpaths, int numpaths, void (*callback)(u_int, int, void *, int *), void *blob)
{
  struct kevent *kes = NULL;
  int kq = -1;
  struct kevent *eventbuff = NULL;
  struct kevent *evt = NULL;
  int count = 0;
  int i = 0;
  int cont = 1;
  int ret = -1;
  int numslashes = -1;
  char *basepath = NULL;
  struct pathinfo *pinfos = NULL;
  struct pathinfo *pinfo = NULL;
  u_int typemask = 0;
  u_int types[] = {NOTE_DELETE,
                   NOTE_WRITE,
                   NOTE_EXTEND,
#ifdef NOTE_TRUNCATE
                   NOTE_TRUNCATE,
#endif
                   NOTE_RENAME};
#ifdef DEBUG_WATCH_PATHS
  char* type_names[] = {"Delete", "Write", "Extend", "Truncate", "Rename"};
#endif
  int numtypes = 0;

  numtypes = sizeof(types)/sizeof(types[0]);
  for(i=0;i < numtypes; i++){
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

  eventbuff = reallocarray(NULL, numpaths+1, sizeof(struct kevent)); /* include space for an error event */
  if(eventbuff == NULL){
    report_error("Unable to allocate event storage");
    goto ERR;
  }

  for( i=0; i< numpaths; i++){
    pinfos[i].index = i;
    pinfos[i].path = NULL;
    if(!inpaths[i]){
      errno = EINVAL;
      report_error("NULL pathname provided to watch_paths");
      goto ERR;
    }
    if(inpaths[i][0]=='/'){
      pinfos[i].path = strndup(inpaths[i], PATH_MAX);
      if(pinfos[i].path == NULL){
        report_error("Unable to allocate space for path of file to watch");
        goto ERR;
      }
    } else {
      if(basepath == NULL) {
        basepath = getcwd(NULL,PATH_MAX);
        if(basepath == NULL) {
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
    pinfos[i].endslash = pinfos[i].slashes + numslashes;
    pinfos[i].nextslash = pinfos[i].slashes;
    pinfos[i].dev = -1;
    EV_SET(&kes[i], -1, EVFILT_VNODE, EV_ADD | EV_ONESHOT, typemask, 0, &pinfos[i]);
    pinfos[i].ke = &kes[i];
    pinfos[i].fdp = (long*)&(kes[i].ident);

    if(walk_to_extant_parent(&pinfos[i]) == -1){
       report_error("unable to do parent walk");
       goto ERR;
    }
    BPOINT;
    
    if(*pinfos[i].fdp == -1){
      report_error("unable to open file for watching");
      goto ERR;
    }
  }

  while(cont) {
    BPOINT;
    evt = eventbuff;
    count = kevent(kq, kes, numpaths, evt, numpaths+1, NULL); /* TODO: support timespec from caller */
    if(count > 0){
      for(; evt< &eventbuff[count]; evt++){
        if(evt->flags & EV_ERROR){
          errno = evt->data;
          report_error("error in event list");
          /* exit to stop loops */
          goto ERR;
        } else {
          pinfo = evt->udata;

          if(*pinfo->nextslash) **pinfo->nextslash = 0;
          debug_printf("EVT: %s\n",pinfo->path);
          if(*pinfo->nextslash) **pinfo->nextslash = '/';

          for(i=0;i < numtypes; i++){
            if(evt->fflags & types[i]){
                    debug_printf("--Matched: %s\n", type_names[i]);
            }
          }
          /* NOTE_DELETE might be a new file copied onto the old path, needs work to follow. */
          if(evt->fflags & (NOTE_DELETE | NOTE_RENAME)){
            pinfo->nextslash++;
            if(pinfo->nextslash >= pinfo->endslash){
              debug_printf("Parents deleted to root of device. Giving up.\n");
              goto ERR;
            }
          }

          if(*pinfo->nextslash) { /* *pinfo->nextslash == 0 when examining leaf */
            if(walk_to_extant_parent(pinfo) == -1){
               report_error("unable to do parent walk");
               goto ERR;
            }
            if(*pinfo->fdp == -1) {
              report_error("Unrecoverable error encountered trying to open a file");
              goto ERR;
            }
          }

          if(pinfo->nextslash == pinfo->slashes){
            callback(evt->fflags, pinfo->index, blob, &cont);
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
  if(pinfos) {
    for(i=0;i<numpaths;i++){
      xfree(pinfos[i].slashes);
      xfree(pinfos[i].path);
    }
  }
  xfree(basepath);
  xfree(pinfos);
  xfree(kes);
  xfree(eventbuff);
  return ret;
}





