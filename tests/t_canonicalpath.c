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

#define CP_TRACK_OVERSIZE

int cp_did_oversize = 0;

#include "../canonicalpath.c"
#include <err.h>
#include <assert.h>
#include <string.h>
#include <regex.h>

int
main(int argc, char **argv)
{
  char *base, *path, *expected, *result;
  regex_t reg;
  int bres, pres, ok;

  if(argc < 4){
    printf("USAGE: t_canonicalpath BASE PATH EXPECTED\n");
    return 1;
  }
  base = argv[1];
  path = argv[2];
  expected = argv[3];

  result = canpath(base, path);
  if(result == NULL){
    err(2, "\nGiven '%s' '%s' '%s' : canpath returned NULL",
        base, path, expected);
  }
  if(0 != strncmp(result, expected, PATH_MAX)){
    errx(3, "\nGiven '%s' '%s' '%s' != '%s'",
         base, path, expected, result);
  }
  /*
   * since we didn't exit above, the various strings must have
   * reasonable length
   */

  ok = regcomp(&reg, "(/|^)\\.{1,2}(/|$)|//", REG_EXTENDED);
  assert(0 == ok);
/*@-nullpass@*/
  bres = regexec(&reg, base, 0, NULL, 0);
  pres = regexec(&reg, path, 0, NULL, 0);
/*@=nullpass@*/
  assert(bres == 0 || bres == REG_NOMATCH);
  assert(pres == 0 || pres == REG_NOMATCH);

  if((*path == '/' && pres == REG_NOMATCH) ||
     (pres == REG_NOMATCH && bres == REG_NOMATCH)){
    if(cp_did_oversize != 0){
      errx(5, "\nGiven '%s' '%s' '%s' : Unexpected oversize in canonicalpath",
           base, path, expected);
    }
  } else {
    if(cp_did_oversize == 0){
      errx(4, "\nGiven '%s' '%s' '%s' : Expected oversize in canonicalpath"
           "did not occur", base, path, expected);
    }
  }
/*@-immediatetrans@*/
  regfree(&reg);
/*@=immediatetrans@*/
  return 0;
}
