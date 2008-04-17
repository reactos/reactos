<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="quartz" type="win32dll" baseaddress="${BASEADDRESS_QUARTZ}" installbase="system32" installname="quartz.dll" allowwarnings="true">
	<importlibrary definition="quartz.spec.def" />
	<include base="quartz">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>quartz.c</file>
	<file>quartz.rc</file>
	<file>quartz.spec</file>
</module>
</group>