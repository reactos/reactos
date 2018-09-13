INSTRUCTIONS FOR BUILDING QUILL-ENABLED VERSION OF TRIDENT
----------------------------------------------------------

Siddharth Agrawal 04/08/98
Updated: 09/01/98

Quill no longer implements an external layout engine for Trident. Instead
the plan of record is to port Quill's text features to Trident for IE6.

The changes described in this file apply for building a version of
Trident that has editing support code required by NetDocs.

1) Edit your nt\private\developr\<your name>\setenv.cmd file to
   include "set QUILL=1" and "set TREE_SYNC=1". If you wish to build
   a VC browse file, also include "set BROWSER_INFO=1".
2) Check out mshtml\common.inc and add the lines:

!ifdef TREE_SYNC
C_DEFINES = $(C_DEFINES) /DTREE_SYNC
!endif

   right after the similar define for QUILL.
3) Check out the file mshtml\src\site\base\sources. In your private
   version, delete the lines which mention the following files in
   the SOURCES macro: treeserv.cxx, viewserv.cxx.
4) Check out the file mshtml\src\core\cdbase\sources. In your private
   version, delete the lines which mention the following files in
   the SOURCES macro: base.cxx.
5) In the mshtml\src\site directory, check out the file "dirs". Edit
   dirs to have a line referring to the "lequill" directory right
   before the "text" directory.
6) Check out the file mshtml\src\f3\dll\sources and add the line:

!ifdef QUILL
    $(ROOT)\src\site\lequill\$(O)\lequill.lib \
!endif

   immediately before the reference to text.lib (there are two such
   references). Also, add the line

!ifdef QUILL
    $(ROOT)\src\site\lequill\$(O)\*.sbr \
!endif

    immediately before the reference to text\$(O)\*.sbr.

   Then add a line

         $(ROOT)\src\site\lequill\$(O)\lequill.lib \

   in the LINKLIBS definition, before the line that has
   $(ROOT)\src\site\acc\$(O)\acc.lib (there are two occurrences of
   this too).

7) If you need to deal with cross-syncing the forked Trident files,
   read xsync.txt.

8) Keep the files lequill\quillsite.h and lequill\itreesync.h checked out.
    That way they can be over-written whenever Quill is built fresh.

9) Other instructions from Trident. Here is some email sent by LyleC:

Summary: mshtmenv.bat now automatically supports multiple enlistments of
MSHTML under the same \nt tree.
 
Details: I've changed mshtmenv.bat to set an environment variable MSHTML
which is the name of the directory your mshtml enlistment is in. The perf
batch files now use this variable in the paths they use. The reason for
this is to support multiple enlistments of MSHTML without having to create
a whole new pub tree (\nt\public, \nt\private, etc). Now, if you omit all
arguments to mshtmenv.bat it will pick up your _NTDRIVE, _NTROOT, and
MSHTML drives/directories from the path of the batch file. For example,
if your shortcut invokes c:\nt\private\inet\mshtml\mshtmenv.bat then it
sets _NTDRIVE=c:, _NTROOT=\nt, and MSHTML=mshtml. If you invoke
d:\ie\private\inet\mshtml.fun\mshtmenv.bat, then it sets _NTDRIVE=d:,
_NTROOT=\ie, and MSHTML=mshtml.fun. It will also automatically determine
if you have a multiprocessor machine and set variables accordingly.
 
One additional change: BUILD_DEFAULT now has the mshtml directory name in it.
This means you can create a MYDIRS file in \nt\private\inet that has all of
your MSHTML enlistments in the OPTIONAL_DIRS section. The correct enlistment
will automatically get built when you build from \nt\private. If you have
questions about this works then come ask me.
 
The only other change to make if you want to do this is to update your aliases
(in \private\developr\yourname\cue.pri) to use "%mshtml%" instead of "mshtml".
 
You should not notice any changes if you don't care to use this new feature.
If you want to use it, just set up one shortcut per enlistment which runs
mshtmenv.bat (no arguments) in the appropriate enlistment. Once you create
the above MYDIRS file and update your aliases everything should just work.
 
Lyle
