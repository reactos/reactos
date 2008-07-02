<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msvcrt40" type="win32dll" baseaddress="${BASEADDRESS_MSVCRT40}" installbase="system32" installname="msvcrt40.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="msvcrt40.spec.def" />
	<include base="msvcrt40">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>msvcrt40.c</file>
	<file>msvcrt40.spec</file>
	<library>wine</library>
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
