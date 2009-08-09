<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="appwiz-new" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_APPWIZ}" installbase="system32" installname="appwiz-new.cpl" unicode="yes">
	<importlibrary definition="appwiz.spec" />
	<include base="appwiz-new">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>msvcrt</library>
	<library>ole32</library>
	<library>uuid</library>
	<library>shell32</library>
	<library>msimg32</library>
	<library>gdi32</library>
	<file>appwiz.c</file>
	<file>createlink.c</file>
	<file>appwiz.rc</file>
</module>
