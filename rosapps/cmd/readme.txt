ReactOS Command Line Interface "CMD" version 0.0.4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is the fourth pre-alpha release of CMD.EXE for ReactOS.
It was converted from the FreeDOS COMMAND.COM.


Warning!! Warning!! Warning!!
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This is a pre-alpha version! Many features have not been tested!
Be careful when you use commands that write to your disk drives,
they might destroy your files or the file system!!!


Status
~~~~~~
This is a converted version of FreeDOS COMMAND.COM.
I added some commands from WinNT's CMD.EXE.


New features and improvements
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  - Fixed redirection and piping.
    (E.g. you can use "type > file" now.)
  - Added new error redirections "2>" and "2>>".
    (E.g.: "make 2>error.log")
  - Added CHCP command.
  - Fixed environment handling.
  - New makefile for lcc-win (makefile.lcc).
  - Rewrote DEL and MOVE with a new structure.
  - Improved national language support.
  - Fixed filename completion.


Compiling
~~~~~~~~~
I converted CMD using MS Visual C++ 5.0 and Win95. The included makefile
is just an experimental version.

If you want to compile and test CMD with djgpp, modify the makefile as needed.
I put the CMD sources into [reactos\apps], the makefile is written for that
directory.

If you want to compile and test CMD using another compiler, just create
a new console application project and add all *.c and *.h files to it.
It should compile without an error.


Please report bugs which are not listed above.


Good luck

  Eric Kohl <ekohl@abo.rhein-zeitung.de>




FreeDOS Command Line Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
About
~~~~~
This software is part of the FreeDOS project. Please email
freedos@sunsite.unc.edu for more information, or visit the freedos
archive at "ftp://sunsite.unc.edu/pub/micro/pc-stuff/freedos".  Also,
visit our web page at http://www.freedos.org/.

The command.com web site is at

   http://www.gcfl.net/FreeDOS/command.com/


This software has been developed by the following people:
(listed in approximate chronological order of contributions)

FreeDOS developers:
   normat@rpi.edu (Tim Norman)
   mrains@apanix.apana.org.au (Matt Rains)
   ejeffrey@iastate.edu (Evan Jeffrey)
   Steffen.Kaiser@Informatik.TU-Chemnitz.DE (Steffen Kaiser)
   Svante Frey (sfrey@kuai.se)
   Oliver Mueller (ogmueller@t-online.de)
   Aaron Kaufman (morgan@remarque.berkeley.edu)
   Marc Desrochers (bitzero@hotmail.com)
   Rob Lake (rlake@cs.mun.ca)
   John P. Price <linux-guru@gcfl.net>
   Hans B Pufal <hansp@digiweb.com>

ReactOS developers:
   Eric Kohl <ekohl@abo.rhein-zeitung.de>


Current Features
~~~~~~~~~~~~~~~~
 - environment handling with prompt and path support.
 - directory utilities.
 - command-line history with doskey-like features.
 - batch file processing.
 - input/output redirection and piping.
 - alias support.
 - filename completion (use TAB)
