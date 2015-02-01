#ifndef reallocarray
#define reallocarray(ptr,num,sz) realloc((ptr),(num) * (sz))
#endif

