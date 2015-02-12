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

#ifndef __watchpaths_h_
#define __watchpaths_h_

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

/*
 * Execute a callback whenever the contents of one of the specified
 * paths is modified. The files described by the paths do not need to
 * exist at the time.
 *
 * If one of the files is deleted, its parent directory will be
 * monitored for deletion or modification, and so on, back up to the
 * furthest level of the path that resides on the same device as the
 * original path.
 *
 * ARGUMENTS
 * ---------
 *
 * inpaths:  the array of pathnames to watch
 *
 * numpaths: the count of paths in `inpaths'
 *
 * callback: the function to invoke when one of the watch paths is
 *           modified
 *
 * CALLBACK PARAMETERS
 * -------------------
 *
 * u_int fflags: A bit mask describing which event triggered the
 *               callback See list of fflags defined for EVFILT_VNODE
 *               in kevent(2)
 *
 * int   index:  The index (in inpaths) of the pathname whose
 *               modification triggered the callback invocation
 *
 * void  *data:  The same value as blob. It's an arbitrary value chosen
 *               by the caller to watchpaths() and is unexamined by
 *               watchpaths().
 *
 * int   *cont:  Controls the exit of watchpaths(). If the callback sets
 *               *cont to zero, watchpaths will exit once the callback
 *               returns.
 *
 */
int watchpaths(char **inpaths, int numpaths,
               void (*callback) (u_int, int, void *, int *), void *blob);

#endif /* __watchpaths_h_ */


