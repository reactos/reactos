<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="expand" type="win32cui" installbase="system32" installname="expand.exe" >
	<include base="ReactOS">include/reactos/wine</include>
	<include base="expand">.</include>
	<library>lz32</library>
	<library>setupapi</library>
	<library>user32</library>
	<file>expand.c</file>
	<file>expand.rc</file>
</module>
