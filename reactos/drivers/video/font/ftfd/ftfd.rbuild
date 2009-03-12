<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ftfd" type="kernelmodedll" entrypoint="FtfdEnableDriver@12" baseaddress="${BASEADDRESS_FREETYPE}" installbase="system32" installname="ftfd.dll" crt="libcntpr">
	<importlibrary definition="freetype.def" />
	<include base="freetype2">include</include>
	<library>win32k</library>
	<library>freetype2</library>
	<file>enable.c</file>
	<file>font.c</file>
	<file>glyph.c</file>
	<file>rosglue.c</file>
	<file>sprintf.c</file>
</module>
