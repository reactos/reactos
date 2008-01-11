<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="odbccp32" type="win32dll" baseaddress="${BASEADDRESS_ODBCCP32}" installbase="system32" installname="odbccp32.dll" allowwarnings="true">
	<importlibrary definition="odbccp32.spec.def" />
	<include base="odbccp32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>ole32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>odbccp32.c</file>
	<file>odbccp32.spec</file>
</module>
