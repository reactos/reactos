<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt40" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT40}" installbase="system32" installname="msvcrt40.dll" iscrt="yes">
	<importlibrary definition="msvcrt40.spec" />
	<include base="msvcrt40">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msvcrt40.c</file>
	<library>wine</library>
	<library>kernel32</library>
</module>
</group>
