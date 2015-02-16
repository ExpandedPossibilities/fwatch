#define CP_TRACK_OVERSIZE

int cp_did_oversize = 0;

#include "../canonicalpath.c"
#include <err.h>
#include <assert.h>
#include <string.h>
#include <regex.h>

int main(int argc, char **argv){
  char *base, *path, *expected, *result;
  regex_t reg;
  int bres, pres;

  if(argc < 4 ){
    printf("USAGE: t_canpath BASE PATH EXPECTED\n");
    return 1;
  }
  base = argv[1];
  path = argv[2];
  expected = argv[3];


  result = canpath(base, path);
  if(result == NULL){
    err(2, "canpath returned NULL");
  }
  if(0 != strncmp(result, expected, PATH_MAX)){
    errx(3, "Given '%s' '%s' '%s' != '%s'",
         base, path, expected, result);
  }
  /*
   * since we didn't exit above, the various strings must have
   * reasonable length
   */

  assert(0 == regcomp(&reg, "(/|^)\\.{1,2}(/|$)|//", REG_EXTENDED));
  bres = regexec(&reg, base, 0, NULL, 0);
  pres = regexec(&reg, path, 0, NULL, 0);
  assert(bres == 0 || bres == REG_NOMATCH);
  assert(pres == 0 || pres == REG_NOMATCH);

  if((*path == '/' && pres == REG_NOMATCH) ||
     (pres == REG_NOMATCH && bres == REG_NOMATCH)){
    if(cp_did_oversize != 0) {
      errx(5, "Unexpected oversize in canonicalpath\n"
           "Given '%s' '%s' '%s' == '%s'",
           base, path, expected, result);
    }
  } else {
    if(cp_did_oversize == 0) {
      errx(4, "Expected oversize in canonicalpath did not occur\n"
           "Given '%s' '%s' '%s' == '%s'",
           base, path, expected, result);
    }
  }
  regfree(&reg);

  return 0;
}
