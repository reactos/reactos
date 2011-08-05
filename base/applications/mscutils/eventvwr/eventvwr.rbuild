<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="eventvwr" type="win32gui" installbase="system32" installname="eventvwr.exe" unicode="yes">
	<include base="eventvwr">.</include>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<file>eventvwr.c</file>
	<file>eventvwr.rc</file>
</module>
