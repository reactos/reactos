<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hdwwiz" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_HDWWIZ}" installbase="system32" installname="hdwwiz.cpl" unicode="yes">
	<importlibrary definition="hdwwiz.spec" />
	<include base="hdwwiz">.</include>
	<library>setupapi</library>
	<library>kernel32</library>
	<library>comctl32</library>
	<file>hdwwiz.c</file>
	<file>hdwwiz.rc</file>
</module>
