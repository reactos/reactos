<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="wtsapi32" type="win32dll" baseaddress="${BASEADDRESS_WTSAPI32}" installbase="system32" installname="wtsapi32.dll" allowwarnings="true">
	<importlibrary definition="wtsapi32.spec" />
	<include base="wtsapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>wtsapi32.c</file>
	<file>wtsapi32.spec</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
