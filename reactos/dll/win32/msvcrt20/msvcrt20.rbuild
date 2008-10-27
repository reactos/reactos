<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt20" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT20}" installbase="system32" installname="msvcrt20.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="msvcrt20.spec" />
	<include base="msvcrt20">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>msvcrt20.c</file>
	<file>msvcrt20.spec</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
