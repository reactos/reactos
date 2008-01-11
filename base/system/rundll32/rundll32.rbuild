<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rundll32" type="win32gui" installbase="system32" installname="rundll32.exe">
	<include base="rundll32">.</include>
	<define name="UNICODE" />
	<library>kernel32</library>
	<library>user32</library>
	<file>rundll32.c</file>
	<file>rundll32.rc</file>
</module>
