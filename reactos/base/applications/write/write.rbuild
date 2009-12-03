<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="write" type="win32gui" installbase="system32" installname="write.exe" unicode="yes">
	<include base="write">.</include>
	<library>user32</library>
	<library>gdi32</library>
	<file>write.c</file>
	<file>rsrc.rc</file>
</module>
