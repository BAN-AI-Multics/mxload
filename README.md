# mxload

## Overview

The **mxload** software can read Multics standard-format tapes,
reload data, produce maps, unpack archives, Forum meetings,
message segments, and mailboxes.

## License

Copyright (c) 1988, 1990, 1991, 1993, Oxford Systems, Inc.

All rights reserved.

Reproduction permitted provided this notice is retained.

## Distribution

* `doc`
  * Reference manual source code and manual pages (*troff*)
* `pdf`
  * Documentation and papers (*Portable Document Format*)
* `src`
  * Source code for building the **mxload** software
* `test`
  * Test cases

## Build

* To build **mxload**, type "`make`".
  * This will build:
    * `mxload`
    * `mxmap`
    * `mxarc`
    * `mxmbx`
    * `mxforum`
    * `mxascii`
    * `pr8bit`
        * documentation

Review the `Makefile` contents for additional build information.

## Original Authors

**mxload** is owned and distributed by:

```text
Oxford Systems, Inc.
30 Ingleside Road
Lexington, Massachusetts
U. S. A.     02420-2522

Phone:   + 1 781 863 5549
E-mail:  osibert@siliconkeep.com
```

## Release History

```text
 1.2a1  2021-06-05   Automated  reformatting,   restyling,   source   code
                     normalization,  Markdown  conversion,   and  Makefile
                     generation.  Supports  only POSIX-like UNIX  systems.
                     ALPHA - complaints to Jeff Johnson  <trnsz@pobox.com>

 1.1.1  2004-11-15   Released  for  general  use.   No  software  changes.

 1.1    1993-10-31   Major  restructuring of  I/O to  support reading from
                     standard  input,  needed  for systems  with poor tape
                     device drivers (specifically, Sequent).  Minor memory
                     management fixes.

 1.0.5  1991-12-20   Bug  fix  for  local  map  generation  ("-l" option).

 1.0.4  1991-12-03   Changed to make WORDALIGN the default in the Makefile
                     (now works out of the box on SPARC, etc.). Bug fix to
                     WORDALIGN gettype.c to declare a missing variable and
                     loop through 8 bytes instead of 9.

 1.0.3  1990-10-22   Bug fix to preserving broken archives as files rather
                     than discarding them. Includes "WORDALIGN" definition
                     in  the Makefile  to  fix  word  alignment  problems.

 1.0.2  1989-10-24   Bug fix for mxmbx handling old (V4) message segments.

 1.0.1  1989-05-26   Bug fix for tape buffer issue, revised documentation.

 1.0    1989-01-13   Initial release.
```

## TODO

* Add test cases, `make test` target
* Fix groff warnings, nroff errors
* Clean up Markdown output and Makefiles
* Direct support for dps8m simulator and SIMH tapes
* Review output of (and appease) ccc-analyzer
