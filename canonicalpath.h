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

#ifndef __canonicalpath_h_
#define __canonicalpath_h_
#include <stddef.h>

/*
 * canonicalpath -- convert a relative path to an absolute path
 *
 * Given a base path and a relative path, returns a path containing no
 * instances of directories named "." or "..".
 *
 * When the base path is omitted, canonicalpath will behave as if the
 * base path had been populated with getcwd(NULL, 0).
 *
 * Both the base path or the relative path may be omitted (passed
 * NULL).
 *
 * If more "../" elements are encountered than there are preceding
 * directories, in the path, extra "../" elements are silently
 * consumed.  This behavior is similar to calling "cd .." from the
 * root of the file system.
 *
 * If `output' is NULL, a newly allocated buffer of sufficient size
 * will be used. This buffer may later be free(3)'d.
 *
 * If `output' is non-NULL, and outlen is smaller than the path to be
 * returned, returns NULL and sets errno to ERANGE. The content of
 * output is undefined in this case. Note that since the real number
 * of bytes needed is not knowable in advance, substantial work may
 * have been done before the size mismatch is discovered.
 *
 * Stores number of bytes actually used in `*used' if `used' is
 * non-NULL.
 *
 * RETURN VALUES
 * -------------
 *
 * If successful, returns a pointer to a canonicalized version of the
 * `rel' relative to `base' (or to the current working directory if
 * `base' is NULL. If any error occurs, returns NULL and ensures that
 * a useful value is present in errno.
 *
 *
 * ERRORS
 * ------
 *
 * [ENAMETOOLONG]    The `base' or `rel' parameters contain no NUL
 *                   bytes within PATH_MAX bytes.
 *
 * [ERANGE]          The `output' parameter was supplied and the
 *                   canonical path is longer than `outlen'
 *
 * canonicalpath may additionally return NULL and set errno to any
 * of the values specified by the library functions malloc(3),
 * realloc(3), or getcwd(3).
 */

/*@null@*/ char* canonicalpath (/*@null@*/ const char *base,
                                /*@null@*/ const char *rel,
                                /*@null@*/ char *output,
                                size_t outlen,
                                /*@null@*/ size_t *used);


/*
 * canpath -- convert a relative path to an absolute path
 *
 * Convenience macro for common use case.
 *
 * See the documentation for canonicalpath for more information
 */
#define canpath(base, rel) canonicalpath((base), (rel), NULL, 0, NULL)

#endif
