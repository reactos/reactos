<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="powrprof" type="win32dll" baseaddress="${BASEADDRESS_POWRPROF}" installbase="system32" installname="powrprof.dll" unicode="yes">
	<importlibrary definition="powrprof.spec" />
	<include base="powrprof">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>powrprof.c</file>
	<file>powrprof.rc</file>
	<file>powrprof.spec</file>
</module>
