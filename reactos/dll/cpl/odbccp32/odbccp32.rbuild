<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="odbccp32i" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_ODBCCP32}" installbase="system32" installname="odbccp32.cpl" unicode="yes">
	<importlibrary definition="odbccp32.spec" />
	<include base="odbccp32">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>odbccp32.c</file>
	<file>odbccp32.rc</file>
	<file>odbccp32.spec</file>
</module>
