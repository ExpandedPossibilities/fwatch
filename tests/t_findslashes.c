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

#include "../watchpaths.c"
#include <err.h>
#include <assert.h>

int main(int argc, char **argv){
  char **slashes;
  char *path, *it;
  size_t count, i, plen;
  if(argc < 2 ){
    printf("USAGE: t_findslashes PATH\n");
    return 1;
  }
  path = argv[1];
  plen = strnlen(argv[1], PATH_MAX);

  slashes = find_slashes(path, 0, &count);
  if(slashes == NULL){
    err(2, "find_slashes returned NULL");
  }
  for(i = 0; i < count; i++){
    /* iterate over slash list and print out the offsets */
    fprintf(stderr, "%zu ", i);
    it = slashes[i];
    if(i == 0){
      assert(it == NULL);
      fprintf(stderr, "---\n");
    } else {
      assert(it != NULL);
      assert(it >= path);
      assert(it <= path + plen);
      fprintf(stderr, "%3ld %.180s\n", it - path, it);
    }
  }
  free(slashes);
  return 0;
}
