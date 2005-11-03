<!-- ...................................................................... -->
<!-- DocBook XML character entities module V4.1.2 ........................... -->
<!-- File dbcentx.mod ..................................................... -->

<!-- Copyright 1992-2000 HaL Computer Systems, Inc.,
     O'Reilly & Associates, Inc., ArborText, Inc., Fujitsu Software
     Corporation, Norman Walsh, and the Organization for the Advancement
     of Structured Information Standards (OASIS).

     $Id: dbcentx.mod,v 1.1 2001/10/09 17:54:26 jfilby Exp $

     Permission to use, copy, modify and distribute the DocBook XML DTD
     and its accompanying documentation for any purpose and without fee
     is hereby granted in perpetuity, provided that the above copyright
     notice and this paragraph appear in all copies.  The copyright
     holders make no representation about the suitability of the DTD for
     any purpose.  It is provided "as is" without expressed or implied
     warranty.

     If you modify the DocBook XML DTD in any way, except for declaring and
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

     <!ENTITY % dbcent PUBLIC
     "-//OASIS//ENTITIES DocBook XML Character Entities V4.1.2//EN"
     "dbcentx.mod">
     %dbcent;

     See the documentation for detailed information on the parameter
     entity and module scheme used in DocBook, customizing DocBook and
     planning for interchange, and changes made since the last release
     of DocBook.
-->

<!-- ...................................................................... -->

<!ENTITY % ISOamsa.module "INCLUDE">
<![%ISOamsa.module;[
<!ENTITY % ISOamsa PUBLIC
"ISO 8879:1986//ENTITIES Added Math Symbols: Arrow Relations//EN//XML"
"ent/iso-amsa.ent">
%ISOamsa;
<!--end of ISOamsa.module-->]]>

<!ENTITY % ISOamsb.module "INCLUDE">
<![%ISOamsb.module;[
<!ENTITY % ISOamsb PUBLIC
"ISO 8879:1986//ENTITIES Added Math Symbols: Binary Operators//EN//XML"
"ent/iso-amsb.ent">
%ISOamsb;
<!--end of ISOamsb.module-->]]>

<!ENTITY % ISOamsc.module "INCLUDE">
<![%ISOamsc.module;[
<!ENTITY % ISOamsc PUBLIC
"ISO 8879:1986//ENTITIES Added Math Symbols: Delimiters//EN//XML"
"ent/iso-amsc.ent">
%ISOamsc;
<!--end of ISOamsc.module-->]]>

<!ENTITY % ISOamsn.module "INCLUDE">
<![%ISOamsn.module;[
<!ENTITY % ISOamsn PUBLIC
"ISO 8879:1986//ENTITIES Added Math Symbols: Negated Relations//EN//XML"
"ent/iso-amsn.ent">
%ISOamsn;
<!--end of ISOamsn.module-->]]>

<!ENTITY % ISOamso.module "INCLUDE">
<![%ISOamso.module;[
<!ENTITY % ISOamso PUBLIC
"ISO 8879:1986//ENTITIES Added Math Symbols: Ordinary//EN//XML"
"ent/iso-amso.ent">
%ISOamso;
<!--end of ISOamso.module-->]]>

<!ENTITY % ISOamsr.module "INCLUDE">
<![%ISOamsr.module;[
<!ENTITY % ISOamsr PUBLIC
"ISO 8879:1986//ENTITIES Added Math Symbols: Relations//EN//XML"
"ent/iso-amsr.ent">
%ISOamsr;
<!--end of ISOamsr.module-->]]>

<!ENTITY % ISObox.module "INCLUDE">
<![%ISObox.module;[
<!ENTITY % ISObox PUBLIC
"ISO 8879:1986//ENTITIES Box and Line Drawing//EN//XML"
"ent/iso-box.ent">
%ISObox;
<!--end of ISObox.module-->]]>

<!ENTITY % ISOcyr1.module "INCLUDE">
<![%ISOcyr1.module;[
<!ENTITY % ISOcyr1 PUBLIC
"ISO 8879:1986//ENTITIES Russian Cyrillic//EN//XML"
"ent/iso-cyr1.ent">
%ISOcyr1;
<!--end of ISOcyr1.module-->]]>

<!ENTITY % ISOcyr2.module "INCLUDE">
<![%ISOcyr2.module;[
<!ENTITY % ISOcyr2 PUBLIC
"ISO 8879:1986//ENTITIES Non-Russian Cyrillic//EN//XML"
"ent/iso-cyr2.ent">
%ISOcyr2;
<!--end of ISOcyr2.module-->]]>

<!ENTITY % ISOdia.module "INCLUDE">
<![%ISOdia.module;[
<!ENTITY % ISOdia PUBLIC
"ISO 8879:1986//ENTITIES Diacritical Marks//EN//XML"
"ent/iso-dia.ent">
%ISOdia;
<!--end of ISOdia.module-->]]>

<!ENTITY % ISOgrk1.module "INCLUDE">
<![%ISOgrk1.module;[
<!ENTITY % ISOgrk1 PUBLIC
"ISO 8879:1986//ENTITIES Greek Letters//EN//XML"
"ent/iso-grk1.ent">
%ISOgrk1;
<!--end of ISOgrk1.module-->]]>

<!ENTITY % ISOgrk2.module "INCLUDE">
<![%ISOgrk2.module;[
<!ENTITY % ISOgrk2 PUBLIC
"ISO 8879:1986//ENTITIES Monotoniko Greek//EN//XML"
"ent/iso-grk2.ent">
%ISOgrk2;
<!--end of ISOgrk2.module-->]]>

<!ENTITY % ISOgrk3.module "INCLUDE">
<![%ISOgrk3.module;[
<!ENTITY % ISOgrk3 PUBLIC
"ISO 8879:1986//ENTITIES Greek Symbols//EN//XML"
"ent/iso-grk3.ent">
%ISOgrk3;
<!--end of ISOgrk3.module-->]]>

<!ENTITY % ISOgrk4.module "INCLUDE">
<![%ISOgrk4.module;[
<!ENTITY % ISOgrk4 PUBLIC
"ISO 8879:1986//ENTITIES Alternative Greek Symbols//EN//XML"
"ent/iso-grk4.ent">
%ISOgrk4;
<!--end of ISOgrk4.module-->]]>

<!ENTITY % ISOlat1.module "INCLUDE">
<![%ISOlat1.module;[
<!ENTITY % ISOlat1 PUBLIC
"ISO 8879:1986//ENTITIES Added Latin 1//EN//XML"
"ent/iso-lat1.ent">
%ISOlat1;
<!--end of ISOlat1.module-->]]>

<!ENTITY % ISOlat2.module "INCLUDE">
<![%ISOlat2.module;[
<!ENTITY % ISOlat2 PUBLIC
"ISO 8879:1986//ENTITIES Added Latin 2//EN//XML"
"ent/iso-lat2.ent">
%ISOlat2;
<!--end of ISOlat2.module-->]]>

<!ENTITY % ISOnum.module "INCLUDE">
<![%ISOnum.module;[
<!ENTITY % ISOnum PUBLIC
"ISO 8879:1986//ENTITIES Numeric and Special Graphic//EN//XML"
"ent/iso-num.ent">
%ISOnum;
<!--end of ISOnum.module-->]]>

<!ENTITY % ISOpub.module "INCLUDE">
<![%ISOpub.module;[
<!ENTITY % ISOpub PUBLIC
"ISO 8879:1986//ENTITIES Publishing//EN//XML"
"ent/iso-pub.ent">
%ISOpub;
<!--end of ISOpub.module-->]]>

<!ENTITY % ISOtech.module "INCLUDE">
<![%ISOtech.module;[
<!ENTITY % ISOtech PUBLIC
"ISO 8879:1986//ENTITIES General Technical//EN//XML"
"ent/iso-tech.ent">
%ISOtech;
<!--end of ISOtech.module-->]]>

<!-- End of DocBook XML character entity sets module V4.1.2 ................. -->
<!-- ...................................................................... -->
