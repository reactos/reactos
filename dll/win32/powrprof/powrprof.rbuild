<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="powrprof" type="win32dll" baseaddress="${BASEADDRESS_POWRPROF}" installbase="system32" installname="powrprof.dll" unicode="yes">
	<importlibrary definition="powrprof.spec" />
	<include base="powrprof">.</include>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>powrprof.c</file>
	<file>powrprof.rc</file>
</module>
