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
