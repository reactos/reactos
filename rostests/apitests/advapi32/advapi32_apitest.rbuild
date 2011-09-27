<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="advapi32_apitest" type="win32cui" installbase="bin" installname="advapi32_apitest.exe">
	<include base="advapi32_apitest">.</include>
	<library>wine</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>pseh</library>
	<library>advapi32</library>
	<file>testlist.c</file>
	<file>CreateService.c</file>

</module>
</group>
