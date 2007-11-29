<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cryptdll" type="win32dll" baseaddress="${BASEADDRESS_CRYPTDLL}" installbase="system32" installname="cryptdll.dll" allowwarnings="true">
	<importlibrary definition="cryptdll.spec.def" />
	<include base="cryptdll">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>cryptdll.c</file>
	<file>cryptdll.spec</file>
</module>
