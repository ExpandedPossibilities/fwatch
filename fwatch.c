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

#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <assert.h>

#include "watchpaths.h"
#include "reallocarray.h"


#ifdef S_SPLINT_S
/*
 * When running splint, the default definitions for these
 * functions result in improper warnings
 */

/*@noreturn@*/ void err(int, const char *, ...);
/*@noreturnwhenfalse@*/ void assert(int);
#endif

/* struct runinfo
 *
 * This structure describes how to invoke the utility.  A structure of
 * this type is initialized in main() and a pointer to it is passed to
 * runscript() as the "data" parameter.
 *
 * c_argc: number of arguments in c_argv
 * c_argv: template argument list
 * files: list of files being watched
 * replace: index of argument in c_argv to replace with the filename
 */
struct runinfo {
  int c_argc;
  /*@NULL@*/ /*@dependent@*/ char **c_argv;
  /*@NULL@*/ /*@dependent@*/ char **files;
  int replace;
};

/*
 * Callback function invoked by watchpaths()
 * See documentation in watchpaths.h for more information.
 */
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
      /*
       * Replace the placeholder in c_argv with a pointer to the
       * pathname of the file whose modification triggered the
       * callback
       */
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

    /*
     * if the utility exited with a code other than zero, tell
     *  watchpaths to stop watching
     */
    if(exitcode != 0){
      *cont = 0;
    }
  }
}

static void
usage()
{
  printf("Usage: fwatch utility [argument ...] ';' file [file2 ...]\n"
         "       fwatch utility [argument ...] '{}' [argument ...] ';'"
         " file [file2 ...]\n\n"
         "Watches files for modification.\n"
         "Invokes utility with configured arguments each time one of the"
         " listed files is modified.\n"
         "Stops watching the files and exits once utility exits with a"
         " return code other than zero.\n"
         "Searches $PATH for utility. Pass a the full path to utility to"
         " avoid this behavior.\n\n"
         "ARGUMENTS\n"
         " Utility will be invoked with arguments from the argument list.\n"
         " A single '{}' in the argument list will be replace with the name"
         " of the modified file.\n"
         " This replacement happens at most once.\n"
         " The semicolon between the argument list and the file list is"
         " mandatory.\n\n"
         "FILES\n"
         " Handles file deletion and deletion of any parent directories by"
         " monitoring for them to\n"
         " be replaced.\n"
         " Treats file renaming as deletion\n"
         " Will continue to monitor the target paths so long as a single"
         " directory in the path\n"
         " exists on the same device.\n\n"
         "EXAMPLES\n"
         " fwatch hexdump -C {} ';' /some/file/that/changes\n"
         " fwatch pfctl -t me -T replace self \\;"
         " /var/db/dhclient.leases.*\n");
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

  /*
   * Consume arguments until encountering "{}", ";", or the end of the
   * string. If "{}" is encountered, its index is stored in
   * info.replace. The last instance wins.
   *
   * The utility arguments are constructed in this fashion rather than
   * simply taking a single string in order to avoid either invoking
   * the shell or otherwise exposing the user to command injection
   * vulnerabilities. Using an array for the arguments allows the use
   * of execvp instead of system.
   */
  for(i = 1; i<argc && ! (argv[i][0] == ';' && argv[i][1] == '\0'); i++){
    if(argv[i][0] == '{' && argv[i][1] == '}' && argv[i][2] == '\0'){
      info.replace = info.c_argc;
    }
    info.c_argc++;
  }

  /*
   * If ";" was not encountered, the argument list is improperly
   * constructed. Show the usage message and exit.
   */

  if(i == argc){
    usage();
    return 1;
  }

  /* All arguments after the semicolon are paths to watch */
  info.files = &argv[info.c_argc + 2];
  fcount = argc - info.c_argc - 2;

  info.c_argv = reallocarray(NULL, info.c_argc + 1, sizeof(char *));
  if(info.c_argv == NULL){
    err(2, "Unable to allocate argument array");
  }

  for(i = 0; i < info.c_argc; i++){
    if(i == info.replace){
      /* this is the placeholder element */
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

  /* invoke runscript() whenever a path in info.files is modified */
  return watchpaths(info.files, fcount, runscript, &info);
}
