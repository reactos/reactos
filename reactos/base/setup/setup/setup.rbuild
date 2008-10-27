<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="setup" type="win32gui" installbase="system32" installname="setup.exe" unicode="yes">
	<include base="setup">.</include>
	<library>kernel32</library>
	<library>userenv</library>
	<file>setup.c</file>
	<file>setup.rc</file>
</module>
