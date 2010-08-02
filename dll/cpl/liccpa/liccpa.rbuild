<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="liccpa" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_LICCPA}"  installbase="system32" installname="liccpa.cpl" unicode="yes">
	<importlibrary definition="liccpa.def" />
	<include base="liccpa">.</include>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<file>liccpa.c</file>
	<file>liccpa.rc</file>
</module>
