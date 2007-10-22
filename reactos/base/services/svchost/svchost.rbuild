<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="svchost" type="win32cui" installbase="system32" installname="svchost.exe">
	<include base="svchost">.</include>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>advapi32</library>
	<file>svchost.c</file>
	<file>svchost.rc</file>
</module>
