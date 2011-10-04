## Copyright (c) 2002-2005, International Business Machines Corporation 
## and others. All Rights Reserved.

This directory contains sample code
Below is a short description of the contents of this directory.

break - demonstrates how to use BreakIterators in C and C++.

cal      - prints out a calendar. 

case    - demonstrates how to do Unicode case conversion in C and C++.

date     - prints out the current date, localized. 

datefmt  - an exercise using the date formatting API

layout   - demonstrates the ICU LayoutEngine

legacy   - demonstrates using two versions of ICU in one application

msgfmt   - demonstrates the use of the Message Format

numfmt   - demonstrates the use of the number format

props    - demonstrates the use of Unicode properties

strsrch - demonstrates how to search for patterns in Unicode text using the usearch interface.

translit - demonstrates the use of ICU transliteration

uciter8.c - demonstrates how to leniently read 8-bit Unicode text.

ucnv     - demonstrates the use of ICU codepage conversion

udata    - demonstrates the use of ICU low level data routines

ufortune - demonstrates packaging and use of resources in an application

ugrep  - demonstrates ICU Regular Expressions. 

uresb    - demonstrates building and loading resource bundles

ustring  - demonstrates ICU string manipulation functions


==
* Where can I find more sample code?

 - The "uconv" utility is a full-featured command line application.
    It is normally built with ICU, and is located in icu/source/extra/uconv

 - The "icuapps" CVS module contains other applications and libraries not
    included with ICU.  You can check it out from the CVS command line
    by using for example,  "cvs co icuapps" instead of "cvs co icu",
   or through WebCVS at http://dev.icu-project.org/cgi-bin/viewcvs.cgi/icuapps/

==
* How do I build the samples?

 - See the Readme in each subdirectory

 To build all samples at once:

    Unix:   - build and install (make install) ICU
            - be sure 'icu-config' is accessible from the PATH
            - type 'make all-samples' from this directory 
               (other targets:  clean-samples, check-samples)
           Note: 'make all-samples' won't work correctly in out of source builds.

            - legacy and layout are not included in these lists,
                   please see their individual readmes.
