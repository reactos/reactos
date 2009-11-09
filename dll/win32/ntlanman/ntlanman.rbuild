<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ntlanman" type="win32dll" baseaddress="${BASEADDRESS_NTLANMAN}" installbase="system32" installname="ntlanman.dll" unicode="yes">
	<importlibrary definition="ntlanman.spec" />
	<include base="ntlanman">.</include>
	<library>kernel32</library>
	<library>netapi32</library>
	<library>ntdll</library>
	<library>wine</library>
	<file>ntlanman.c</file>
	<file>ntlanman.rc</file>
</module>
