#ifndef __canonicalpath_h_
#define __canonicalpath_h_
#include <stddef.h>

char* canpath (const char *base, const char *rel);

char* canonicalpath (const char *base, const char *rel,
                     char* output, size_t outlen, size_t* used);

#endif
