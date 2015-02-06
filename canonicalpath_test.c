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

#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>
#include <err.h>
#include "canonicalpath.h"

#ifdef S_SPLINT_S
/*@noreturn@*/ void err(int, const char *, ...);
#endif

int
main(int argc, char **argv)
{
  char *pth = NULL;
  char *cwd = NULL;

  if(argc < 1){
    printf("At least one argument is required\n");
    return 2;
  }

/*@-nullpass@*/
  cwd = getcwd(NULL, 0);
/*@=nullpass@*/

  pth = canonicalpath(cwd, argv[1], NULL, 0, NULL);
  if(pth == NULL) err(1, "not sure what happened!");
  printf("%s\n", pth);

  free(pth);

  pth = canonicalpath(argv[1], NULL, NULL, 0, NULL);
  if(pth == NULL) err(1, "not sure what happened!");
  printf("%s\n", pth);

  free(pth);
  return 0;
}
