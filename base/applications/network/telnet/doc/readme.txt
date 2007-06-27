              **************************************************
              ** Console Telnet v2.1b2 README.TXT 16 Oct 2000 **
              **************************************************

                               RELEASE NOTES:
                               --------------

This release of TELNET is a beta one. This means that it is working as far
as it is tested, and has a few bugs. Hopefully this will be a stable
version.  Please send comments and bug reports to me at
pbranna@clemson.edu, or to the mailing list (see below).  See file
CHANGES.TXT for a detailed log of changes.  See file BUGS.TXT for known
bugs.

                                DESCRIPTION:
                                ------------

This is a telnet client with full color ANSI support for Windows NT/95
console. You can use this program from the Win95 command line (MsDos) and
run it in full screen text mode. You may also redirect the telnet session
to STDIN and STDOUT for use with other programs.  Telnet will communicate
the number of lines and rows to the host, and can operate in any console
mode.  Most of it's options are customizable.


                         COPYRIGHT/LICENSE/WARRANTY
                         --------------------------

Telnet Win32, Copyright (C) 1996-1997, Brad Johnson <jbj@nounname.com>
Copyright (C) 1998 I.Ioannou, Copyright (C) 1999-2000 Paul Brannan.  Telnet
is a free project released under the GNU public license.  This program comes
with ABSOLUTELY NO WARRANTY.  This is free software, and you are welcome
to redistribute it under the licence contitions.  See LICENSE.TXT for
details.

                                REQUIREMENTS:
                                -------------

This program requires a Microsoft Win32 enviroment (Windows 95/98/NT) with
Winsock TCP/IP.  16 bit Win3.x or Win32s are not supported.

                                  FEATURES:
                                  ---------

Full ANSI colors and (almost) complete ANSI emulation.
User configurable options via telnet.ini.
User configurable key bindings with alternative keyboards.
Icoming character translations.
Redirection of telnet session.
Telnet output can be dumped to a file.
Local printer support.
Basic scrollback support.
Basic VT emulation.
Mouse support.
Clipboard (cut-and-paste) support.
Support for multiple screen sizes.

                              WHERE TO GET IT:
                              ----------------

Since version 2.0, Console Telnet's new home page is
http://www.musc.edu/~brannanp/telnet/.  You can get the latest version from
ftp://argeas.cs-net.gr/Telnet-Win32 or from the web page.  Telnet is
available as full project (sources included) or as binaries only.  If you
would like to help to the development check the /devel directory on the ftp
site for a recent alpha version.

                                MAILING LIST:
                                -------------

Telnet has it's own mailing list for announcements, bug reports, support,
suggestions etc. To subscribe send e-mail to majordomo@argeas.cs-net.gr
with empty Subject, and the word subscribe in the body. List's address is
telnet-win32@argeas.cs-net.gr You can find the old archives at
http://www.cs-net.gr/lists

If you are only interested in announcements, follow the above procedures to
subscribe to telnet-win32-announce.  The development list is
telnet-win32-devel.

                                HOW TO HELP:
                                ------------

Telnet is a free project made from volunteers. If you know C/C++ and would
like to help in the development you are welcome :-)  Just contact
pbranna@clemson.edu, and/or subscribe to the mailing list.  Check
ftp://argeas.cs-net.gr/Telnet-Win32/devel for a recent alpha version.


                                INSTALLATION
                                ------------

Just copy telnet.exe, telnet.ico, telnet.ini and keys.cfg to a directory.
I prefer a directory included in the PATH (such as C:\WINDOWS, but this will
overwrite the telnet that comes with Windows -- which is not necessarily a
bad thing). If you are upgrading from a previous version please look below
(Key file definitions) : the keys.cfg file has changed a bit. Also look at
the Configuration section below, TELNET now has a ini file.

                                   USAGE:
                                   -------

  TELNET
    Begins telnet and enters telnet> command line.

  TELNET [params][host [port]]
    Connects to port on host. Port defaults to 23 for TELNET.

    params    -d FILENAME.EXT    Dumps all incoming data to FILENAME.EXT
                                 Note lowercase 'd'.
              --variable=value   Overrides ini variable to be set to value.

    host                         Host name or IP to connect to
    port                         Service port to open connection on
                                 (default is telnet port 23).

  TELNET -?
    Gives usage information.

Pressing the escape key (default ALT-]) will break out of a telnet session and
return you to the telnet> prompt. Pressing return will resume your session.
All the options are available from the telnet> prompt. Type ? to get help.

Pressing the scrollback key (default ALT-[) will give you a basic scrollback
view. Pressing ESC will resume your session.

                                   BUGS:
                                   -----

There are :-). Hopefully this version is more stable than the previous. See
BUGS.TXT, and grep for FIX ME's in the sources. Any help ?

                                   NOTES:
                                   ------

If the environment variable LANG has a valid value (e.g. LANG=de for German
characters) and the file LOCALE.DLL is installed somewhere along the PATH
TELNET will not ignore local characters.

If you have problems with paste under Win 95 try unchecking the fast paste
option in the MsDos properties. The paste function works correctly under NT.
This is a Microsoft bug :-)

                                CONFIGURATION
                                -------------

The configuration is made through telnet.ini and keys.cfg. These files (at
least telnet.ini) must be in the same directory which telnet.exe is.  The
basic options are loaded from the file telnet.ini. If you are having problems
with a terminal setting, check the file OPTIONS.TXT for configuration
information.


Key file definitions (telnet.cfg)
-------------------------------

Use the key file (telnet.cfg) to define the characters that telnet is sending
to the host.  From version 2b5 you can configure the output keys (KEYMAP
sections), the input character translations (CHARMAP sections) and you can
combine all to as many configurations as you like (CONFIG sections). You
can also have alternative keymaps in a configuration, and keys to switch
between them.  See the comments in keys.cfg for details.

NOTE: if you are upgrading from a previous version you must put your old keys
in the KEYMAP sections.
Please send any national specific keymaps / charmaps / configurations to be
included to the next version.


                              HOW TO COMPILE IT
                             -------------------

Telnet compiles with a variety of compilers.  You will need at least
Borland 4.x or newer compiler, or MSVC 2.0 or newer, or download a version
of gcc for Win32 (see http://www.musc.edu/~brannanp/telnet/gccwin32.html).
Copy the files from the directories BORLAND or MSVC to the main directory,
change them to fit to your system, and recompile.  The project comes with
IDE files and makefiles.

Follow the instructions for your compiler to compile telnet.  A Makefile
for use with mingw32 or other gcc variants has been included, so if you have
gcc, you can just type "make" at the command line.

                               SPECIAL THANKS:
                               ---------------

Many people have worked for this project. Please forgive me (and let me
know!) if I have forgotten anyone. We all thank them :-)

Igor Milavec <igor.milavec@uni-lj.si>
        Original Author of version 1.1
        Igor wrote the basic telnet program and released it to public.

Brad Johnson <jbj@nounname.com>  http://nounname.com
        Author of versions 2.0b to 2b4. Brad has wrote plenty of code for
        telnet like ansi colors, emulation, scrollback option, and many
        others.

Titus_Boxberg@public.uni-hamburg.de
        Ansi emulation improvements
        German keyboard configuration

I.Ioannou roryt@hol.gr
        KeyTranslator class (version 2b3)
        Maintainer (since version 2b5)

Andrei V. Smilianets <smile@head.aval.kiev.ua> (version 2b5)
        KeyTranslator class (version 2b5)
        Prompt improvments

Paul Brannan <pbranna@clemson.edu>
        Telnet.ini author, MSVC port, speed improvements, VT support,
        and many others.
        Maintainer (since version 2b6)

Leo Leibovici <leo.leibovici@nouveau.co.uk>
        Fixed some crashes in the ANSI parser
        Wrote UK keymap

Dmitry Lapenkov <dl@bis.msk.su>
        Wrote AT386 keymaps
        Improved telnet icon

Thomas Briggs <tbriggs@qmetric.com>
        Fixed problem with Ctrl-Break
        Added suspend and fast quit options to the command line
        Error messages for unable to load ini file
        Fixed bug w/ getting name of executable

BK Oxley
        Fixed TELNET_INI environment variable

Sam Robertson
        Fixed compilation problems with MSVC6
        Bugfix with telnet crashing at exit

Vassili Bourdo <vassili_bourdo@softhome.net>
        Keyboard initialization improvements

Craig Davidson <crn@ozemail.com.au>
        Bugfixes for telnet prompt
        Added suspend telnet option
        Set port number using name rather than number

Pedro Gutierrez <paag@coppi.tid.es>
        Save/restore console title
        Bugfix w/ character mapping

Daniel Straub <Daniel.Straub@nbgm.siemens.de>
        Bugfix with telnet crashing at exit

Jose Cesar Otero Rodriguez <jcotero@las.es>
        Spanish Keyboard definition
        Cursor size sequences

Bryan Montgomery <monty@english.net>
        Added CtrlBreak_as_CtrlC option
        Added Scroll_Enable option

Adi Seiker
        Added Set_Title ini file option

Craig Nellist
        Updated Winsock error messages
        Sleeping while thread paused, to give up CPU time
        Command-line history

Jakub Sterba
        Czech keyboard definition

Ziglio Frediano
        MTE (Meridian Terminal) Support

Mark Miesfield
	Fixed redirection
	Wrote documentation for redirection

---

Paul Brannan <pbranna@clemson.edu>
