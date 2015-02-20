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

#include "../canonicalpath.c"
#include <err.h>
#include <assert.h>
#include <string.h>


#define bufflen 9

int main(/*@unused@*/ int argc, /*@unused@*/ char **argv){
  char data[] = "1234567890";
  char *base, *path, *result;
  char buff[bufflen + 1];
  size_t used = 0;

  buff[bufflen] = '\0';

  base = data + 5;
  path = data;

  result = canpath(base, path);
  if(result != NULL || errno != EINVAL){
    errx(1, "Failed to detect overlapping base and path");
  }

  base = malloc(PATH_MAX + 3);
  assert(base != NULL);
  memset(base, (int)'x', PATH_MAX + 1);
  base[PATH_MAX + 2] = '\0';

  errno = 0;
  result = canpath(base, "some/path");
  if(result != NULL || errno != ENAMETOOLONG){
    err(2, "Failed to detect too-long input");
  }

  errno = 0;
  result = canonicalpath("/a/base", "some/path", buff, 0, &used);
  if(result != NULL || errno != ERANGE){
/*@-nullpass@*/
    err(3, "Failed to detect too-small output buffer 1 %p", result);
/*@=nullpass@*/
  }

  errno = 0;
  result = canonicalpath("/foo", "../../../../", buff, 0, &used);
  if(result != NULL || errno != ERANGE){
/*@-nullpass@*/
    err(4, "Failed to detect too-small output buffer 2 %p", result);
/*@=nullpass@*/
  }

  errno = 0;
  result = canonicalpath("/foo", "../../../../", buff, 1, &used);
  if(result != NULL || errno != ERANGE){
/*@-nullpass@*/
    err(5, "Failed to detect too-small output buffer 3 %p", result);
/*@=nullpass@*/
  }

  errno = 0;
  result = canonicalpath("/foo", "../../../../", buff, 2, &used);
  if(result != buff || used != 1){
/*@-nullpass@*/
    err(6, "Failed to manage too-many ..'s %p %p %zd", result, buff, used);
/*@=nullpass@*/
  }

  free(base);
  return 0;
}
