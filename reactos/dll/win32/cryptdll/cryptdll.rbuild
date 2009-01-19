<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cryptdll" type="win32dll" baseaddress="${BASEADDRESS_CRYPTDLL}" installbase="system32" installname="cryptdll.dll">
	<importlibrary definition="cryptdll.spec" />
	<include base="cryptdll">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cryptdll.c</file>
</module>
