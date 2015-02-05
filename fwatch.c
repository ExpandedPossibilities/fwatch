/*
 * Copyright (c) 2015, Expanded Possibilities, Inc. <ek_fwatch@expandedpossibilities.com>
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
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <err.h>
#include <assert.h>

#include "watchpaths.h"
#include "reallocarray.h"


#ifdef S_SPLINT_S
/*@noreturn@*/ void err(int, const char *, ...);
/*@noreturnwhenfalse@*/ void assert(int);
#endif

struct runinfo {
  int c_argc;
  /*@NULL@*/ /*@dependent@*/ char **c_argv;
  /*@NULL@*/ /*@dependent@*/ char **files;
  int replace;
};

static void
runscript(/*@unused@*/ u_int flags, int idx, void *data, int *cont)
{
  pid_t pid;
  int status = 0, exitcode = 0;
  struct runinfo *info = data;

#ifdef FW_DEBUG
  char **dumper;

  printf("forking\n");
#endif

  assert(info != NULL);
  assert(info->files != NULL);
  assert(info->c_argv != NULL);

  pid = fork();
  if(pid == 0){
    if(info->replace >= 0){
      info->c_argv[info->replace] = info->files[idx];
    }

#ifdef FW_DEBUG
    printf("exec\n");
    for(dumper = info->c_argv; *dumper; dumper++){
      printf(" %s\n", *dumper);
    }
    printf("\n");
#endif

    (void) execvp(info->c_argv[0], info->c_argv);

    err(2, "failed to exec '%s'", info->c_argv[0]); /* should not reach */
  } else {
    (void) waitpid(pid, &status, 0);
    exitcode = WEXITSTATUS(status);

#ifdef FW_DEBUG
    printf("Exit Code: %d\n", exitcode);
#endif

    if(exitcode != 0){
      *cont = 0;
    }
  }
}

static void
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
main(int argc, char **argv)
{
  int i;
  struct runinfo info = {0, NULL, NULL, -1};
  int fcount;
  char *arg;

  if(argc < 2){
    usage();
    return 1;
  }

  for(i = 1; i<argc && ! (argv[i][0] == ';' && argv[i][1] == '\0'); i++){
    if(argv[i][0] == '{' && argv[i][1] == '}' && argv[i][2] == '\0'){
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

  info.c_argv = reallocarray(NULL, info.c_argc + 1, sizeof(char *));
  if(info.c_argv == NULL){
    err(2, "Unable to allocate argument array");
  }
  for(i = 0; i < info.c_argc; i++){
    if(i == info.replace){
      arg = NULL;
    } else {
      arg = strdup(argv[i + 1]);
      if(arg == NULL){
        err(2, "Unable to allocate space for argument element");
      }
    }
    info.c_argv[i] = arg;
  }
  info.c_argv[info.c_argc] = NULL;

#ifdef FW_DEBUG
  printf("ready:");
  for(i = 0; i < fcount; i++){
    printf(" %s", info.files[i]);
  }
  printf("\n");
#endif

  return watchpaths(info.files, fcount, runscript, &info);
}



