<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hh" type="win32gui" installbase="system32" installname="hh.exe" unicode="no">
	<include base="hh">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>main.c</file>
	<file>hh.rc</file>
</module>
