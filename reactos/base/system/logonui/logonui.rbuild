<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="logonui" type="win32gui" installbase="system32" installname="LogonUI.exe" unicode="yes">
	<include base="logonui">.</include>
	<library>user32</library>
	<library>gdi32</library>
	<file>logonui.c</file>
	<file>NT5design.c</file>
	<file>NT6design.c</file>
	<file>logonui.rc</file>
</module>
