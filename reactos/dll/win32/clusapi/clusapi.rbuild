<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="clusapi" type="win32dll" baseaddress="${BASEADDRESS_CLUSAPI}" installbase="system32" installname="clusapi.dll" allowwarnings="true">
	<importlibrary definition="clusapi.spec" />
	<include base="clusapi">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>clusapi.c</file>
	<file>clusapi.spec</file>
</module>
