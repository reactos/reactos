<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="spoolsv" type="win32cui" installbase="system32" installname="spoolsv.exe" unicode="yes">
	<include base="spoolsv">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>spoolsv.c</file>
	<file>spoolsv.rc</file>
</module>
