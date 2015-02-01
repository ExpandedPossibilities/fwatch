#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <err.h>
#include "watchpaths.h"
#include "reallocarray.h"

struct runinfo {
  int c_argc;
  char** c_argv;
  char** files;
  int replace;
};

void
runscript(u_int flags, int idx, void *data, int *cont)
{
  pid_t pid;
  int status = 0, exitcode = 0;
  struct runinfo *info = data;

#ifdef DEBUG
  char** dumper;

  printf("forking\n");
#endif

  pid = fork();
  if(pid == 0){
    if(info->replace >= 0){
      info->c_argv[info->replace] = info->files[idx];
    }

#ifdef DEBUG
    printf("exec\n");
    for(dumper = info->c_argv;*dumper;dumper++){
      printf(" %s\n",*dumper);
    }
    printf("\n");
#endif

    execvp(info->c_argv[0],info->c_argv);
    
    err(2, "failed to exec '%s'",info->c_argv[0]); /* //should not reach */
  } else {
    waitpid(pid, &status, 0);
    exitcode = WEXITSTATUS(status);

#ifdef DEBUG
    printf("Exit Code: %d\n",exitcode);
#endif

    if(exitcode != 0) {
      *cont = 0;
    }
  }
}

void
usage()
{
  printf("Usage: fwatch utility [argument ...] ';' file [file2 ...]\n"
         "       fwatch utility [argument ...] '{}' [argument ...] ';' file [file2 ...]\n\n"
         "Watches files for modification.\n"
         "Invokes utility with configured arguments each time one of the listed files is modified.\n"
         "Stops watching the files and exits once utility exits with a return code other than zero.\n\n"
         "ARGUMENTS\n"
         " Utility will be invoked with arguments from the argument list.\n"
         " A single '{}' in the argument list will be replace with the name of the modified file.\n"
         " This replacement happens at most once.\n"
         " The semicolon between the argument list and the file list is mandatory.\n\n"
         "FILES\n"
         " Handles file deletion and deletion of any parent directories by monitoring for them to\n"
         " be replaced.\n"
         " Treats file renaming as deletion\n"
         " Will continue to monitor the target paths so long as a single directory in the path\n"
         " exists on the same device.\n\n"
         "EXAMPLES\n"
         " fwatch hexdump -C {} ';' /some/file/that/changes\n"
         " fwatch pfctl -t me -T replace self \\; /var/db/dhclient.leases.*\n");
}

int
main(int argc, char** argv)
{
  int i;
  struct runinfo info = { 0, NULL, NULL, -1 };
  int fcount;
  char* arg;

  if(argc <2){
    usage();
    return 1;
  }

  for(i=1; i<argc && ! (argv[i][0] == ';' && argv[i][1] == 0); i++) {
    if(argv[i][0] == '{' && argv[i][1] == '}' && argv[i][2] == 0){
      info.replace = info.c_argc;
    }
    info.c_argc++;
  }

  if(i == argc){
    usage();
    return 1;
  }

  info.files = &argv[info.c_argc + 2];
  fcount = argc - info.c_argc - 2;

  info.c_argv = reallocarray(NULL, info.c_argc + 1, sizeof(char*));
  if(info.c_argv == NULL){
    err(2, "Unable to allocate argument array");
  }
  for(i=0; i<info.c_argc;i++){
    if(i == info.replace) {
      arg = NULL;
    } else {
      arg = strdup(argv[i+1]);
      if(arg == NULL){
        err(2, "Unable to allocate space for argument element");
      }
    }
    info.c_argv[i] = arg;
  }
  info.c_argv[info.c_argc] = NULL;

#ifdef DEBUG
  printf("ready:");
  for(i=0;i<fcount;i++){
    printf(" %s", info.files[i]);
  }
  printf("\n");
#endif
  return watch_paths(info.files, fcount, runscript, &info);
}



