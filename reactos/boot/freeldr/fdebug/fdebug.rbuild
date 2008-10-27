<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="freeldr_fdebug" type="win32gui" installbase="system32" installname="fdebug.exe" unicode="yes">
	<include base="freeldr_fdebug">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>comdlg32</library>
	<library>gdi32</library>
	<file>fdebug.c</file>
	<file>rs232.c</file>
	<file>fdebug.rc</file>
</module>
