Until a better solution is found (like extending cabman) please recreate vgafont.bin
with 7-Zip or a similar compression tool. The file has to be in standard zip format without
compression. (CompressionMethod = 0)

sources:

http://opengrok.creo.hu/dragonfly/xref/src/share/syscons/fonts/cp437-8x8.fnt
http://opengrok.creo.hu/dragonfly/xref/src/share/syscons/fonts/cp850-8x8.fnt
http://opengrok.creo.hu/dragonfly/xref/src/share/syscons/fonts/cp865-8x8.fnt
http://opengrok.creo.hu/dragonfly/xref/src/share/syscons/fonts/cp866-8x8.fnt
http://packages.debian.org/en/etch/all/console-data/ file: gr737-8x8.psf
http://packages.debian.org/en/etch/all/console-data/ file: lat2-08.psf (as a base), remade several
 glyphs (by GreatLord), corrected the aligning and cleaned up some of the fonts, finally changed from
 ISO to DOS ASCII standard CP852 (by Caemyr)
http://cman.us/files/cp775-8x8.zip