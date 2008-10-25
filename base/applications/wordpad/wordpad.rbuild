<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wordpad" type="win32gui" installbase="system32" installname="wordpad.exe" allowwarnings="true">
	<include base="wordpad">.</include>
	<library>comdlg32</library>
	<library>shell32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>msvcrt</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>comctl32</library>
	<compilationunit name="unit.c">
		<file>print.c</file>
		<file>registry.c</file>
		<file>wordpad.c</file>
	</compilationunit>
	<file>rsrc.rc</file>
</module>
