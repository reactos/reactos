<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt_apitest" type="win32cui" installbase="bin" installname="msvcrt_apitest.exe">
	<include base="msvcrt_apitest">.</include>
	<library>wine</library>
	<library>pseh</library>
	<library>msvcrt</library>
	<file>testlist.c</file>

	<file>ieee.c</file>
	<file>splitpath.c</file>

</module>
</group>
