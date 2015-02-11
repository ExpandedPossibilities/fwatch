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

#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>

#include "canonicalpath.h"

#ifdef S_SPLINT_S
/*@noreturn@*/ void err(int, const char *, ...);
#endif

int
main(int argc, char **argv)
{
  char *pth = NULL;
  char *base = NULL;
  int i;

  if(argc < 2
     || strncmp(argv[1],"--help", 6) == 0
     || strncmp(argv[1],"-h", 2) == 0){
    printf("USAGE: canname [BASE] PATH [PATH2 ...]\n"
           "Writes a canonicalized version of each PATH, relative to current\n"
	   "working directory or BASE, to standard output.\n");
    return 2;
  }

  if(argc >= 3){
    base = argv[1];
  }

  for( i = base == NULL ? 1 : 2; i < argc ; i++) {
    pth = canpath(base, argv[i]);
    if(pth == NULL) err(1, "Failed to calculate canonical path");
    printf("%s\n", pth);
    free(pth);
  }
  return 0;
}
