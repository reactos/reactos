February, 1995

D-Flat Version 20

The source files in the DFLAT archive constitute the D-Flat windowing
system. This is public domain code. You may use it in your
applications without restriction. You may freely distribute source
code. It would be nice if you would give credit to Dr. Dobb's Journal
as the original publisher of the source code.

The software build procedure makes a program named memopad.exe. It is
a multiple-document notepad program. Observe the #define VERSION
statement in dflat.h. The version number should correspond with the
nn in the DFLTnn.ZIP filename that you downloaded. Check the uploads
periodically to see if there is a more recent version available.

For other examples download the Quincy and IMail applications. The
IMail application uses the DFWRAP C++ class wrapper.

My CompuServe ID is 71101,1262. I monitor the DDJFORUM every day and
prefer that you discuss D-Flat there so that every one can benefit
from your comments.

========== Borland C++ 3.1 or 4.0 ============

To build with the Borland C++ 3.1 or 4.x make utility:

Set the DRIVE macro in the makefile to the drive and directory 
where your compiler is installed.

Type this command:

    C>make /fmakefile.bcc

=======================================================

D-Flat uses Compressed help files. It uses an adaptation of the
Huffman compression programs from the Dr. Dobb's Journal C
Programming Column of early 1991. 

The makefile compresses the help file with these commands:

   C>huffc memopad.txt memopad.hlp
   C>fixhelp memopad.hlp

Both of the programs must be run in order for the help system to work
properly. Note that .hlp files from previous versions are not
compatible with versions 18 and higher. The .txt files are, however,
so you can use the huffc and fixhelp programs to rebuild the .hlp
file.

===============================================

This is version 20. It incorporates these changes:

1. Dialog boxes displayed twice upon open.

2. The intended #define of FA_DIREC in direct.c was inadvertently
   released with #include as the preprocessing directive.

3. Changed status bar to display time after screen takeover.

4. Statbar incorrectly captured clock with every MOVE message. Caused
   hangups when compiled with BCC 3.1 and user moved application window.

5. Modeless dialog boxes did not redisplay properly when there was
more than one window open.

6. Added ButtonEnabled and CheckBoxEnabled functions.

7. Changed LB_SETSELECTION message to accept -1 to turn off
selections in the listbox.

8. Moved processing of ID_SEARCH, ID_SEARCHNEXT, ID_REPLACE, ID_CUT,
ID_COPY, ID_PASTE, ID_DELETETEXT, ID_UNDO, and ID_CLEAR to editbox.c,
relieving applications of these common processes.

9. Now allow marking blocks, cut, copy, paste, etc. on single line
editboxes.

10. Added SetProtect function.

11. The Window menu listed doc windows that were not visible.

12. Changed class identifiers to Class.

13. ClearWindow did not work properly for a window with a status bar
and menu bar and no title bar.

14. Added the void *wrapper field to the window structure to support
a C++ class wrapper around the D-Flat C function library.

15. Sizeable windows with borders now display a small token in the
lower right corner to indicate that they are sizeable.

===============================================

Buy the DDJ CD-ROM from Miller-Freeman to get the complete D-Flat
narrative from May '91 to October '92. These columns describe the
development of D-Flat and serve as a tutorial on its use. The D-Flat
reference manual is DFLAT.DOC.



