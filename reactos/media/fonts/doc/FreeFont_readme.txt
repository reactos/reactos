-*-text-*-
                          GNU FreeFont

The GNU FreeFont project aims to provide a useful set of free scalable
(i.e., OpenType) fonts covering as much as possible of the ISO 10646/Unicode
UCS (Universal Character Set).

Statement of Purpose
--------------------

The practical reason for putting glyphs together in a single font face is
to conveniently mix symbols and characters from different writing systems,
without having to switch fonts.

Coverage
--------

FreeFont covers the following character sets

* ISO 8859 parts 1-15
* CEN MES-3 European Unicode Subset
  http://www.evertype.com/standards/iso10646/pdf/cwa13873.pdf
* IBM/Microsoft code pages 437, 850, 852, 1250, 1252 and more
* Microsoft/Adobe Windows Glyph List 4 (WGL4)
  http://www.microsoft.com/typography/otspec/WGL4.htm
* KOI8-R and KOI8-RU
* DEC VT100 graphics symbols
* International Phonetic Alphabet
* Arabic, Hebrew, Armenian, Georgian, Ethiopian and Thai alphabets,
  including Arabic presentation forms A/B
* mathematical symbols, including the whole TeX repertoire of symbols
* APL symbols
  etc.

Editing
-------

The free outline font editor, George Williams's FontForge
<http://fontforge.sourceforge.net/> is used for editing the fonts.

Design Issues
-------------

Which font shapes should be made?  Historical style terms like Renaissance
or Baroque letterforms cannot be applied beyond Latin/Cyrillic/Greek
scripts to any greater extent than Kufi or Nashki can be applied beyond
Arabic script; "italic" is really only meaningful for Latin letters. 

However, most modern writing systems have typographic formulations for
contrasting uniform and modulated character stroke widths, and have some
history with "oblique", faces.  Since the advent of the typewriter, most
have developed a typographic style with uniform-width characters.

Accordingly, the FreeFont family has one monospaced - FreeMono - and two
proportional faces (one with uniform stroke - FreeSans - and one with
modulated stroke - FreeSerif).

To make text from different writing systems look good side-by-side, each
FreeFont face is meant to contain characters of similar style and weight.

Licensing
---------

Free UCS scalable fonts is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

The fonts are distributed in the hope that they will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

As a special exception, if you create a document which uses this font, and
embed this font or unaltered portions of this font into the document, this
font does not by itself cause the resulting document to be covered by the
GNU General Public License. This exception does not however invalidate any
other reasons why the document might be covered by the GNU General Public
License. If you modify this font, you may extend this exception to your
version of the font, but you are not obligated to do so.  If you do not
wish to do so, delete this exception statement from your version.


Files and their suffixes
------------------------

The files with .sfd (Spline Font Database) are in FontForge's native format. 
Please use these if you plan to modify the font files.

TrueType fonts for immediate consumption are the files with the .ttf
(TrueType Font) suffix.  These are ready to use in Xwindows based
systems using FreeType, on Mac OS, and on older Windows systems.

OpenType fonts (with suffix .otf) are for use in Windows Vista. 
Note that although they can be installed on Linux, but many applications
in Linux still don't support them.


--------------------------------------------------------------------------
Primoz Peterlin, <primoz.peterlin@biofiz.mf.uni-lj.si>
Steve White <stevan.white@googlemail.com>

Free UCS scalable fonts: http://savannah.gnu.org/projects/freefont/
$Id: README,v 1.7 2009/01/13 08:43:23 Stevan_White Exp $
