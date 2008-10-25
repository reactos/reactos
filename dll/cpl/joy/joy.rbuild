<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="joy" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_JOY}"  installbase="system32" installname="joy.cpl" unicode="yes">
	<importlibrary definition="joy.def" />
	<include base="joy">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<file>joy.c</file>
	<file>joy.rc</file>
</module>
