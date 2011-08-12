<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="thmsvc" type="win32cui" installbase="system32" installname="thmsvc.exe" unicode="yes">
	<include base="thmsvc">.</include>
	<library>uxtheme</library>
	<library>wine</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<file>thmsvc.c</file>
	<file>thmsvc.rc</file>
</module>
