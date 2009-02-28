<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="usp10" type="win32dll" baseaddress="${BASEADDRESS_USP10}" installbase="system32" installname="usp10.dll" allowwarnings="true">
	<importlibrary definition="usp10.spec" />
	<include base="usp10">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>usp10.c</file>
	<library>wine</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
