<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="bootok" type="win32cui" installbase="system32" installname="bootok.exe" unicode="yes">
	<include base="bootok">.</include>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>bootok.c</file>
	<file>bootok.rc</file>
</module>
