<!-- ...................................................................... -->
<!-- DocBook XML notations module V4.1.2 .................................... -->
<!-- File dbnotnx.mod ..................................................... -->

<!-- Copyright 1992-2000 HaL Computer Systems, Inc.,
     O'Reilly & Associates, Inc., ArborText, Inc., Fujitsu Software
     Corporation, Norman Walsh, and the Organization for the Advancement
     of Structured Information Standards (OASIS).

     $Id: dbnotnx.mod,v 1.1 2001/10/09 17:54:26 jfilby Exp $

     Permission to use, copy, modify and distribute the DocBook XML DTD
     and its accompanying documentation for any purpose and without fee
     is hereby granted in perpetuity, provided that the above copyright
     notice and this paragraph appear in all copies.  The copyright
     holders make no representation about the suitability of the DTD for
     any purpose.  It is provided "as is" without expressed or implied
     warranty.

     If you modify the DocBook DTD in any way, except for declaring and
     referencing additional sets of general entities and declaring
     additional notations, label your DTD as a variant of DocBook.  See
     the maintenance documentation for more information.

     Please direct all questions, bug reports, or suggestions for
     changes to the docbook@lists.oasis-open.org mailing list. For more
     information, see http://www.oasis-open.org/docbook/.
-->

<!-- ...................................................................... -->

<!-- This module contains the entity declarations for the standard ISO
     entity sets used by DocBook.

     In DTD driver files referring to this module, please use an entity
     declaration that uses the public identifier shown below:

     <!ENTITY % dbnotn PUBLIC
     "-//OASIS//ENTITIES DocBook XML Notations V4.1.2//EN"
     "dbnotnx.mod">
     %dbnotn;

     See the documentation for detailed information on the parameter
     entity and module scheme used in DocBook, customizing DocBook and
     planning for interchange, and changes made since the last release
     of DocBook.
-->

<!ENTITY % local.notation.class "">
<!ENTITY % notation.class
		"BMP| CGM-CHAR | CGM-BINARY | CGM-CLEAR | DITROFF | DVI
		| EPS | EQN | FAX | GIF | GIF87a | GIF89a 
		| JPG | JPEG | IGES | PCX
		| PIC | PNG | PS | SGML | TBL | TEX | TIFF | WMF | WPG
		| linespecific
		%local.notation.class;">

<!NOTATION BMP		PUBLIC
"+//ISBN 0-7923-9432-1::Graphic Notation//NOTATION Microsoft Windows bitmap//EN">
<!NOTATION CGM-CHAR	PUBLIC "ISO 8632/2//NOTATION Character encoding//EN">
<!NOTATION CGM-BINARY	PUBLIC "ISO 8632/3//NOTATION Binary encoding//EN">
<!NOTATION CGM-CLEAR	PUBLIC "ISO 8632/4//NOTATION Clear text encoding//EN">
<!NOTATION DITROFF	SYSTEM "DITROFF">
<!NOTATION DVI		SYSTEM "DVI">
<!NOTATION EPS		PUBLIC 
"+//ISBN 0-201-18127-4::Adobe//NOTATION PostScript Language Ref. Manual//EN">
<!NOTATION EQN		SYSTEM "EQN">
<!NOTATION FAX		PUBLIC 
"-//USA-DOD//NOTATION CCITT Group 4 Facsimile Type 1 Untiled Raster//EN">
<!NOTATION GIF		SYSTEM "GIF">
<!NOTATION GIF87a               PUBLIC
"-//CompuServe//NOTATION Graphics Interchange Format 87a//EN">

<!NOTATION GIF89a               PUBLIC
"-//CompuServe//NOTATION Graphics Interchange Format 89a//EN">
<!NOTATION JPG		SYSTEM "JPG">
<!NOTATION JPEG		SYSTEM "JPG">
<!NOTATION IGES		PUBLIC 
"-//USA-DOD//NOTATION (ASME/ANSI Y14.26M-1987) Initial Graphics Exchange Specification//EN">
<!NOTATION PCX		PUBLIC 
"+//ISBN 0-7923-9432-1::Graphic Notation//NOTATION ZSoft PCX bitmap//EN">
<!NOTATION PIC		SYSTEM "PIC">
<!NOTATION PNG          SYSTEM "http://www.w3.org/TR/REC-png">
<!NOTATION PS		SYSTEM "PS">
<!NOTATION SGML		PUBLIC 
"ISO 8879:1986//NOTATION Standard Generalized Markup Language//EN">
<!NOTATION TBL		SYSTEM "TBL">
<!NOTATION TEX		PUBLIC 
"+//ISBN 0-201-13448-9::Knuth//NOTATION The TeXbook//EN">
<!NOTATION TIFF		SYSTEM "TIFF">
<!NOTATION WMF		PUBLIC 
"+//ISBN 0-7923-9432-1::Graphic Notation//NOTATION Microsoft Windows Metafile//EN">
<!NOTATION WPG		SYSTEM "WPG"> <!--WordPerfect Graphic format-->
<!NOTATION linespecific	SYSTEM "linespecific">

<!-- End of DocBook XML notations module V4.1.2 ............................. -->
<!-- ...................................................................... -->
