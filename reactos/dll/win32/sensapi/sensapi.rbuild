<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sensapi" type="win32dll" baseaddress="${BASEADDRESS_SENSAPI}" installbase="system32" installname="sensapi.dll" allowwarnings="true">
	<importlibrary definition="sensapi.spec" />
	<include base="sensapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>sensapi.c</file>
	<file>sensapi.spec</file>
</module>
