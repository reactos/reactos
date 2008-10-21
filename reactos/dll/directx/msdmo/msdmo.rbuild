<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msdmo" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MSDMO}" installbase="system32" installname="msdmo.dll" allowwarnings="true">
	<importlibrary definition="msdmo.spec" />
	<include base="msdmo">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>msdmo.c</file>
	<file>msdmo.rc</file>
	<file>msdmo.spec</file>
</module>
</group>
