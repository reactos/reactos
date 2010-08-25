<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dciman32_apitest" type="win32cui" installbase="bin" installname="dciman32_apitest.exe">
	<include base="dciman32_apitest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>pseh</library>
	<file>testlist.c</file>

	<file>DCICreatePrimary.c</file>

</module>
</group>
