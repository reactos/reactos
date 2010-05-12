<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="telnetd" type="win32cui" installbase="system32" installname="telnetd.exe" unicode="no">
	<include base="reactos"></include>
	<include base="telnetd">..</include>

	<library>ntdll</library>
	<library>advapi32</library>
	<library>ws2_32</library>
	<library>wine</library>

	<file>telnetd.c</file>
	<file>serviceentry.c</file>
	<file>telnetd.rc</file>
</module>
