#ifndef __canonicalpath_h_
#define __canonicalpath_h_
#include <stddef.h>

/*@null@*/ char* canpath (/*@null@*/ const char *base,
                          /*@null@*/ const char *rel);

/*@null@*/ char* canonicalpath (/*@null@*/ const char *base,
                                /*@null@*/ const char *rel,
                                /*@null@*/ char *output,
                                size_t outlen,
                                /*@null@*/ size_t *used);

#endif
