<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bootok" type="win32cui" installbase="system32" installname="bootok.exe" unicode="yes">
	<include base="bootok">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>bootok.c</file>
	<file>bootok.rc</file>
</module>
