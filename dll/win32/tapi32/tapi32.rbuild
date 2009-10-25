<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="tapi32" type="win32dll" baseaddress="${BASEADDRESS_TAPI32}" installbase="system32" installname="tapi32.dll" allowwarnings="true" entrypoint="0">
	<importlibrary definition="tapi32.spec" />
	<include base="tapi32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>assisted.c</file>
	<file>internal.c</file>
	<file>line.c</file>
	<file>phone.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
