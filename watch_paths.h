#ifndef __watch_paths_h_
#define __watch_paths_h_

#define u_int unsigned int

#ifndef reallocarray
#define reallocarray(ptr,num,sz) reallocf((ptr),(num) * (sz))
#endif 

int      watch_paths(char **inpaths, int numpaths, void (*callback)(u_int, int, void *, int *), void *blob);

#endif /* __watch_paths_h_ */


