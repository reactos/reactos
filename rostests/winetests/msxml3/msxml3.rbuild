<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msxml3_winetest" type="win32cui" installbase="bin" installname="msxml3_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="msxml3_winetest">.</include>
	<file>domdoc.c</file>
	<file>saxreader.c</file>
	<file>schema.c</file>
	<file>testlist.c</file>
	<file>xmldoc.c</file>
	<file>xmlelem.c</file>
	<library>wine</library>
	<library>user32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
