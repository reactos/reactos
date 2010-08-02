ReactOS command line interpreter CMD
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The ReactOS command line interpreter CMD is derived from FreeCOM, the
FreeDOS command line interpreter.

We are shooting mainly to be just like 2000/XP cmd.exe.  They are very close and only a small number(none that i can recall off the top of my head, so maybe 0) differences have been found between those two.  It has been reported that ROS cmd.exe does not work on nt4 because of a missing api.  I'm hoping to fix this at some point.


Compiling
~~~~~~~~~
ROS cmd used to depend on __REACTOS__ to provide two different ways to build cmd.  There is still code left in it for this but...  The __REACTOS__ = 0 has not been develped, maintained.  And therefore it does not even compile anymore.  __REACTOS__ = 1 works fine on both windows(nt). and someday i plan to remove all the __REACTOS__ = 0.

Using rbuild you can compile cmd seperatly by "make cmd_install".  Also you can compile cmd using MSVC 6 and soon 7/8 hopefully.


Current Features
~~~~~~~~~~~~~~~~
 - environment handling with prompt and path support.
 - directory utilities.
 - command-line history with doskey-like features.
 - batch file processing.
 - input/output redirection and piping.
 - alias support.
 - filename completion (use TAB), both unix and windows style.


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
   Eric Kohl
   Emanuele Aliberti <ea@iol.it>
   Paolo Pantaleo <paolopan@freemail.it>
   Brandon Turner <turnerb7@msu.edu>



Bugs
~~~~
There is still many bugs ;)
Please report bugs to ReactOS team <ros-dev@reactos.org> or to bugzilla at www.reactos.org

