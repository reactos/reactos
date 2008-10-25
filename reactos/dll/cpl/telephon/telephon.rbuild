<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="telephon" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_TELEPHON}"  installbase="system32" installname="telephon.cpl" unicode="yes">
	<importlibrary definition="telephon.def" />
	<include base="telephon">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<file>telephon.c</file>
	<file>telephon.rc</file>
</module>
