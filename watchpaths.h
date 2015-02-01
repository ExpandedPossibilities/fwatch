#ifndef __watch_paths_h_
#define __watch_paths_h_

#include <sys/types.h>

#ifndef u_int
#ifdef uint32_t
#define u_int uint32_t
#else
#define u_int unsigned int
#endif
#endif

#ifndef u_short
#ifdef uint16_t
#define u_short uint16_t
#else
#define u_short unsigned short
#endif
#endif

int      watch_paths(char **inpaths, int numpaths, void (*callback)(u_int, int, void *, int *), void *blob);

#endif /* __watch_paths_h_ */

