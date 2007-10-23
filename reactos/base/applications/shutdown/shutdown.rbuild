<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="shutdown" type="win32cui" installbase="system32" installname="shutdown.exe">
	<include base="shutdown">.</include>
	<define name="WINVER">0x0501</define>
	<library>advapi32</library>
	<library>user32</library>
	<library>kernel32</library>
	<file>misc.c</file>
	<file>shutdown.c</file>
	<file>shutdown.rc</file>
	<pch>precomp.h</pch>
</module>
