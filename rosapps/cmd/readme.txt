ReactOS command line interpreter CMD version 0.1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ReactOS command line interpreter CMD is derived from FreeCOM, the
FreeDOS command line interpreter.

We are going for 4NT compatibility but try to stay compatible with
WinNT's CMD.EXE too.


Compiling
~~~~~~~~~
Cmd can be built in two different versions. A full version for use under
Windows 9x or Windows NT and a reduced version for use under ReactOS.

Note: The full version won't runder ReactOS and the reduced version is not
usable under Win 9x/NT.

To build the full version, make sure the symbol '__REACTOS__' is NOT defined
in 'rosapps/cmd/config.h' line 13.

To build the reduced version, make sure the symbol '__REACTOS__' is defined
in 'rosapps/cmd/config.h' line 13.


Current Features
~~~~~~~~~~~~~~~~

 - environment handling with prompt and path support.
 - directory utilities.
 - command-line history with doskey-like features.
 - batch file processing.
 - input/output redirection and piping.
 - alias support.
 - filename completion (use TAB)


Credits
~~~~~~~

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
   Emanuele Aliberti <ea@iol.it>
   Paolo Pantaleo <paolopan@freemail.it>


Bugs
~~~~

Please report bugs to Eric Kohl <ekohl@abo.rhein-zeitung.de>.


Good luck

  Eric Kohl
