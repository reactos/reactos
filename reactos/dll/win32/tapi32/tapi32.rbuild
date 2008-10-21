<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="tapi32" type="win32dll" baseaddress="${BASEADDRESS_TAPI32}" installbase="system32" installname="tapi32.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="tapi32.spec" />
	<include base="tapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>assisted.c</file>
	<file>line.c</file>
	<file>phone.c</file>
	<file>tapi32.spec</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
