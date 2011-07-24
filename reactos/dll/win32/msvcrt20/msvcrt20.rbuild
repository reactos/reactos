<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt20" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT20}" installbase="system32" installname="msvcrt20.dll" allowwarnings="true" entrypoint="0" iscrt="yes">
	<importlibrary definition="msvcrt20.spec" />
	<include base="msvcrt20">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="CRTDLL" />
	<file>msvcrt20.c</file>
    <file>stubs.c</file>
	<library>wine</library>
    <library>crt</library>
    <library>pseh</library>
</module>
</group>
