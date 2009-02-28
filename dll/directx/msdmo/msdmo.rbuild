<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="msdmo" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MSDMO}" installbase="system32" installname="msdmo.dll">
	<importlibrary definition="msdmo.spec" />
	<include base="msdmo">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>msdmo.c</file>
	<file>msdmo.rc</file>
</module>
</group>
