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

#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <assert.h>

#include "../watchpaths.h"
#include "../splint_defs.h"


static char **files;

static void
callback(/*@unused@*/ u_int flags, int idx, void *data, int *cont)
{
  struct stat finfo;
  int *count = data;

  if(--(*count) <= 0) *cont = 0; /* last iteration */

  assert(files[idx] != NULL);
  printf("%s", files[idx]);
  if(0 != fflush(stdout)){
    err(20, "Unable to flush");
  }

  if(-1 == stat(files[idx], &finfo)){
    printf("\n");
    err(100 + idx, "Failed to stat %s", files[idx]);
  }

  printf(" %zd\n", finfo.st_size);
  if(0 != fflush(stdout)){
    err(20, "Unable to flush");
  }
}

int
main(int argc, char **argv)
{
  int count, ret;

  if(argc < 3){
    errx(1, "USAGE: t_watchpaths TIMES FILE [FILE ...]\n");
  }

  count = atoi(argv[1]);
  assert(count > 0);

  files = argv + 2;
  printf("STARTING\n");
  if(0 != fflush(stdout)){
    err(20, "Unable to flush");
  }
  ret = watchpaths(files, argc - 2, callback, &count);
  if(0 != ret){
    err(2, "Error in watchpaths call");
  }
  printf("DONE\n");
  return ret;
}
