<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="powercfg" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_PWRCFG}" installbase="system32" installname="powercfg.cpl" unicode="yes">
	<importlibrary definition="powercfg.spec" />
	<include base="powercfg">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>powrprof</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>advapi32</library>
	<file>powercfg.c</file>
	<file>powershemes.c</file>
	<file>alarms.c</file>
	<file>advanced.c</file>
	<file>hibernate.c</file>
	<file>powercfg.rc</file>
	<file>powercfg.spec</file>
</module>
