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

FreeFont covers the following character ranges
* Latin, Cyrillic, and Arabic, with supplements for many languages
* Greek, Hebrew, Armenian, Georgian, Thaana, Syriac
* Devanagari, Bengali, Gujarati, Gurmukhi, Sinhala, Tamil, Malayalam
* Thai, Tai Le, Kayah Li, Hanunóo, Buginese
* Cherokee, Unified Canadian Aboriginal Syllabics
* Ethiopian, Tifnagh, Vai, Osmanya, Coptic
* Glagolitic, Gothic, Runic, Ugaritic, Old Persian, Phoenician, Old Italic
* Braille, International Phonetic Alphabet
* currency symbols, general punctuation and diacritical marks, dingbats
* mathematical symbols, including much of the TeX repertoire of symbols
* technical symbols: APL, OCR, arrows,
* geometrical shapes, box drawing
* musical symbols, gaming symbols, miscellaneous symbols
  etc.
For more detail see <http://www.gnu.org/software/freefont/coverage.html>

Editing
-------

The free outline font editor, George Williams' FontForge
<http://fontforge.sourceforge.net/> is used for editing the fonts.

Design Issues
-------------

Which font shapes should be made?  Historical style terms like Renaissance
or Baroque letterforms cannot be applied beyond Latin/Cyrillic/Greek
scripts to any greater extent than Kufi or Nashki can be applied beyond
Arabic script; "italic" is strictly meaningful only for Latin letters, 
although many scripts such as Cyrillic have a history with "cursive" and
many others with "oblique" faces. 

However, most modern writing systems have typographic formulations for
contrasting uniform and modulated character stroke widths, and since the
advent of the typewriter, most have developed a typographic style with
uniform-width characters.

Accordingly, the FreeFont family has one monospaced - FreeMono - and two
proportional faces (one with uniform stroke - FreeSans - and one with
modulated stroke - FreeSerif).

The point of having characters from different writing systems in one font
is that mixed text should look good, and so each FreeFont face contains
characters of similar style and weight.

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
They may be used to modify the fonts.

TrueType fonts are the files with the .ttf (TrueType Font) suffix.  These
are ready to use in Linux/Unix, on Apple Mac OS, and on Microsoft Windows
systems.

OpenType fonts (with suffix .otf) are preferred for use on Linux/Unix,
but *not* for recent Microsoft Windows systems.
See the INSTALL file for more information.

Web Open Font Format files (with suffix .woff) are for use in Web sites.
See the webfont_guidelines.txt for further information.

Further information
-------------------

Home page of GNU FreeFont:
	http://www.gnu.org/software/freefont/

More information is at the main project page of Free UCS scalable fonts:
	http://savannah.gnu.org/projects/freefont/

To report problems with GNU FreeFont, it is best to obtain a Savannah
account and post reports using that account on
	https://savannah.gnu.org/bugs/
	
Public discussions about GNU FreeFont may be posted to the mailing list
	freefont-bugs@gnu.org

--------------------------------------------------------------------------
Original author: Primoz Peterlin
Current administrator: Steve White <stevan.white@googlemail.com>

$Id: README,v 1.10 2011-06-12 07:14:12 Stevan_White Exp $
