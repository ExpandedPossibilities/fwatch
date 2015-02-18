# Foreword

This repository provides four things:

 1. `fwatch`: A command-line utility to trigger an action when any one
    of a list of files is modified.
 2. `canname`: A command-line utility in the spirit of `basename(1)` and
    `dirname(1)` which returns the canonical name of its argument
 3. `watchpaths()`: A function for programs to be receive a callback when
    any one of a list of files is modified.
 4. `canonicalpath()` and `canpath()`: Functions for converting relative
    pathnames to absolute pathnames

Both `canname` and `watchpaths()` depend on `canonicalpath()`. The
`fwatch` utility itself depends on `watchpaths()`. Everything in the
repository is provided under the terms of the 2-clause BSD license so
that the components may be included with ease in other projects.

This project was built built for use on OpenBSD to monitor DHCP status
changes and take action based on which lease file was modified. It
also builds on Mac OS X. I presume it builds on other BSDs, but have
not tried. It relies on `kqueue(2)` and will not run on Linux.

See Enrico M. Crisostomo's `fswatch` for a more complete utility which
scratches a similar itch and does run on Linux.


# Building

The base directory contains the source files, the GNUmakefile, the (bsd)
Makefile. *The binaries are built in the obj directory.* A patch to
use `autoconf` or `cmake` is welcome.

Normal: `make`

Running `make` should do the right thing. On OS X this runs the
`GNUmakefile` by default. On OpenBSD it should run bsd make using
`Makefile` as the input.

If you plan to modify the source, consider running `make depend &&
make` so that make knows which header file modifications should
trigger rebuilding.

There are a number of possible flags you may want to pass to
make (e.g via `make SOMEFLAG=1`):

* `RELEASE`: turns on optimization, disables cc warnings & debug symbols
* `ANALYZE`: runs the clang analyzer (GNUmake+clang only)
* `FW_DEBUG`: emit debugging information in `fwatch`
* `WP_DEBUG`: emit debugging information in `watchpaths`
* `WP_COMPLAIN`: emit error information via `perror(3)` in `watchpaths`

Any number of the flags may be used in concert.


# Usage

The documentation for `canonicalpath()`, `canpath()`, and
`watchpaths()` is stored as block comments in `canonicalpath.h` and
`watchpaths.h`.

Usage information for the `fwatch` utility can be seen by running
`fwatch` with no arguments. A simple example follows:

    fwatch /sbin/pfself {} ';' /var/db/dhclient.leases.em0

This invokes the program `/sbin/pfself` (from my router project)
whenever `/var/db/dhclient.leases.em0` is modified, such as by
`dhclient`. If my ISP decides to offer a different IP address, this
program updates two tables used in my firewall configuration, allowing
the firewall rules to operate correctly despite the IP change.


# Dependencies

There are no external runtime dependencies.

Either BSD or GNU make will simplify building. Otherwise, just pass
the `canonicalpath.c`, `watchpaths.c`, and `fwatch.c` files to your
compiler to generate the `fwatch` binary.

A C99 compiler is required unless you delete the variadic debugging
macros in `watchpaths.c`.

`fwatch` should build and run on any BSD-based system with
`kqueue(2)`, which appeared in FreeBSD 4.1. Where possible (OpenBSD
5.6+), `reallocarray` is used rather than bare `realloc`.


# Authorship Process

This project was created without knowledge of the similar
functionality embodied in `fswatch` and the GNU
`canonicalize_file_name(3)` function. It was started while not
connected to the Internet. I failed to find the utility I wanted
search of the documentation on one of my OpenBSD machines. I didn't
want to go online and install a utility from the package repository,
so I wrote my own simple functions.

The source code for this project has been analyzed by using the clang
analyzer which ships with the OS X build tools. It has also been
analyzed by splint and contains splint annotations (which look like:
`/*@word@*/`). These two analyses revealed a small number of actual
bugs, and a slew of warnings requiring annotations to explicitly
define how memory for certain variables was handled. No legitimate
warnings are revealed by either clang or splint at this time. Finally,
some subsections of the code which could run on Linux were analyzed
using klee and the issues found were fixed. The klee analysis is more
thorough than the others, but could only be applied to a small subset
of the codebase.

Early versions of this code used `realpath(3)` instead of my own path
canonicalization code. This worked for watching for the creation of
files in existing directories on OpenBSD, but did not work on OS X due
to a change in the behavior of the function on that platform. The
current version uses `canonicalpath()`, which allows watching for the
creation of entire directory trees in `watchpaths()`.

Not using `realpath(3)` means that backtracking tricks involving
symlinks are not detected. If you attempt to watch `foo/../bar`, then
`watch_paths()` will watch `/bar`, even if `foo` is a symlink deep
into the file system. This is the tradeoff for being able to watch
for paths which do not yet exist. To avoid this behavior, use absolute
paths when invoking `fwatch`, or `watchpaths()`. A future version may
make this behavior optional.

The path canonicalization code attempts to conserve memory and to
operate in as few passes over the input as possible. These goals are
at odds with each other. In all but the simplest case, the input is
read thrice: first to determine string lengths, then to perform the
canonicalization, and finally to shift the output so that the storage
buffer can be resized to the minimum size necessary to hold the output
string. The simple case where no "."  or ".." or runs of slashes are
present results in perfectly sized buffer and does not require a
memmove or realloc. The one-pass canonicalization is performed from
right to left. Each character is examined and pumped through a simple
state machine. Runs of slashes are compressed. Directories named "."
are not copied to the output buffer. Directories named ".." are
similarly not copied, and increment a counter which, when positive,
causes the next non-".." directory to not be copied either. The code
is not portable to systems which do not use "/" as the path separator,
".." to mean go-up-a-level, and "." to mean the current directory.

The `watchpaths()` function is designed to monitor for the modification
of a file at a particular path. It intentionally does not follow files
if they are renamed. If the file does not exist at the onset or is
deleted, watchpaths will monitor the nearest existing parent directory
for the recreation of the missing path elements, though it will not
cross device boundaries. The parent directory monitoring behavior is
supported by the canonical path name. This allows relative paths to be
passed in for monitoring without loss of functionality. `watchpaths()`
uses a callback system to indicate when one of the files under
observation has changed. It is suitable for including in any project
looking for a simplified interface to `kqueue(2)` for monitoring a
particular path.

The `fwatch` utility uses `watchpaths()` to invoke a function which in
turn invokes forks and execs another utility, optionally passing the
pathname of the modified file as an argument to that utility. For
increased safety, no shell is invoked in running the
sub-program. Argument passing is handled using a placeholder `{}` and
a sentinel `;`.


# Bugs

Certain failure conditions can cause fwatch to exit, resulting in no
further calls to the utility even if paths are modified. In
particular, a failure to fork or execute the utility will result in
fwatch exiting.

Passing a NULL argument as the `basepath` in `canonicalpath()` causes
the rather slow `getcwd(3)` to be invoked. This also consumes extra
memory. If you will be using `canonicalpath()` in a loop in your own
code, consider pre-calculating the base path.

Paths which begin with a pair of slashes have special meanings on some
systems. `canonicalpath()` does not treat these specially and will
remove one of the slashes.

fwatch may fail to notice that a parent directory is renamed, but will
reacquire a reference to the correct file should the file itself be
deleted or renamed.

Please report additional bugs to fwatch_bugs@expandedpossibilities.com


# Possible modifications under consideration

Adding a parameter to  watchpaths() to allow the caller to specify
the mode to use in opening the file to monitor and them. This would
allow the open file descriptor to be passed to the callback()
function, avoiding an extraneous call to open(2).

Adding a parameter to  watchpaths() to allow the caller to specify
a timeout to trigger the callback if no modification happens after the
specified timeframe

Modifying watchpaths() to watch simultaneously all parent directories for
modification so that a change further up the path would not be
missed. An older version of watchpaths() did this, but it seemed an
unnecessary complication at the time

Modifying fwatch to build-in daemonization, triggered by a parameter.

Modifying fwatch to echo the list of watched paths when it is first
invoked.


# LICENSE

Copyright (c) 2015, Expanded Possibilities, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

# AUTHOR

Eric Kobrin <eric_kobrin_fwatch@expandedpossibilities.com>
