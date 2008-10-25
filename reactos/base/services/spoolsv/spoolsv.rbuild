<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="spoolsv" type="win32cui" installbase="system32" installname="spoolsv.exe" unicode="yes">
	<include base="spoolsv">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>spoolsv.c</file>
	<file>spoolsv.rc</file>
</module>
