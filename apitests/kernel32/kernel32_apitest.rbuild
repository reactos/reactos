<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="kernel32_apitest" type="win32cui" installbase="bin" installname="kernel32_apitest.exe">
	<include base="kernel32_apitest">.</include>
	<library>wine</library>
	<library>ntdll</library>
	<library>pseh</library>
	<file>testlist.c</file>

	<file>GetDriveType.c</file>
</module>
</group>
