<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="msimg32" type="win32dll" baseaddress="${BASEADDRESS_MSIMG32}" installbase="system32" installname="msimg32.dll" allowwarnings="true">
	<importlibrary definition="msimg32.spec" />
	<include base="msimg32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>msimg32_main.c</file>
</module>
