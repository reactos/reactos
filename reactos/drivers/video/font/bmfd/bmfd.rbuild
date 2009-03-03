<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bmfd" type="kernelmodedriver" entrypoint="FonfdEnableDriver@12" installbase="system32" installname="bmfd.dll" crt="static">
	<library>win32k</library>
	<library>libcntpr</library>
	<file>enable.c</file>
	<file>font.c</file>
	<file>glyph.c</file>
</module>
