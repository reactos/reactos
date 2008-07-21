<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="logoff" type="win32cui" installbase="system32" installname="logoff.exe">
	<include base="logoff">.</include>
	<define name="WINVER">0x0501</define>
	<library>advapi32</library>
	<library>user32</library>
	<library>kernel32</library>
	<file>misc.c</file>
	<file>logoff.c</file>
	<file>logoff.rc</file>
	<pch>precomp.h</pch>
</module>
