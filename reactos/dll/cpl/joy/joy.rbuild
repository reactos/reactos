<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="joy" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_JOY}"  installbase="system32" installname="joy.cpl" unicode="yes">
	<importlibrary definition="joy.spec" />
	<include base="joy">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<file>joy.c</file>
	<file>joy.rc</file>
	<file>joy.spec</file>
</module>
