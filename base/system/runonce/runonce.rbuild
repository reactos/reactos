<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="runonce" type="win32gui" installbase="system32" installname="runonce.exe" unicode="yes">
	<include base="runonce">.</include>
	<library>advapi32</library>
	<library>user32</library>
	<file>runonce.c</file>
	<file>runonce.rc</file>
</module>
