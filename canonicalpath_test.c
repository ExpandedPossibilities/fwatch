#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>
#include <err.h>
#include "canonicalpath.h"

int
main (int argc, char** argv)
{
  char* pth = canonicalpath(getcwd(NULL,PATH_MAX), argv[1], NULL, 0, NULL);
  if(!pth) err(1,"not sure what happened!");
  printf("%s\n", pth );

  pth = canonicalpath(NULL, argv[1], NULL, 0, NULL);
  if(!pth) err(1,"not sure what happened!");
  printf("%s\n", pth );
  return 0;
}
