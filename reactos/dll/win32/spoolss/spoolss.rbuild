<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="spoolss" type="win32dll" baseaddress="${BASEADDRESS_SPOOLSS}" installbase="system32" installname="spoolss.dll" allowwarnings="true">
	<importlibrary definition="spoolss.spec" />
	<include base="spoolss">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>spoolss_main.c</file>
	<library>wine</library>
	<library>winspool</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
