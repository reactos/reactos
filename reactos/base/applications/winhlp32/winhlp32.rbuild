<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winhlp32" type="win32gui" installbase="system32" installname="winhlp32.exe" unicode="no">
	<include base="winhlp32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__ROS_LONG64__" />
    <define name="_CRT_NONSTDC_NO_DEPRECATE" />
	<library>wine</library>
	<library>comdlg32</library>
	<library>comctl32</library>
	<library>shell32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<file>callback.c</file>
	<file>hlpfile.c</file>
	<file>macro.c</file>
	<file>string.c</file>
	<file>winhelp.c</file>
	<file>lex.yy.c</file>
	<file>rsrc.rc</file>
</module>
