<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="format" type="win32cui" installbase="system32" installname="format.exe" allowwarnings="true">
	<include base="format">.</include>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>fmifs</library>
	<file>format.rc</file>
	<file>format.c</file>
</module>
