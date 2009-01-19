<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dciman32" type="win32dll" baseaddress="${BASEADDRESS_DCIMAN32}" installbase="system32" installname="dciman32.dll">
	<importlibrary definition="dciman32.spec" />
	<include base="dciman32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>dciman_main.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
