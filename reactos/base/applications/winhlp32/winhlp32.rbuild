<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="winhlp32" type="win32gui" installname="winhlp32.exe" allowwarnings="true" unicode="no">
	<include base="winhlp32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__ROS_LONG64__" />
    <define name="_CRT_NONSTDC_NO_DEPRECATE" />
	<library>wine</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<file>callback.c</file>
	<file>hlpfile.c</file>
	<file>macro.c</file>
	<file>string.c</file>
	<file>winhelp.c</file>
	<file>lex.yy.c</file>
	<file>Bg.rc</file>
	<file>Cs.rc</file>
	<file>Da.rc</file>
	<file>De.rc</file>
	<file>En.rc</file>
	<file>Es.rc</file>
	<file>Fi.rc</file>
	<file>Fr.rc</file>
	<file>Hu.rc</file>
	<file>It.rc</file>
	<file>Ja.rc</file>
	<file>Ko.rc</file>
	<file>Lt.rc</file>
	<file>Nl.rc</file>
	<file>No.rc</file>
	<file>Pl.rc</file>
	<file>Pt.rc</file>
	<file>Rm.rc</file>
	<file>Ru.rc</file>
	<file>Si.rc</file>
	<file>Sk.rc</file>
	<file>Sv.rc</file>
	<file>Tr.rc</file>
	<file>rsrc.rc</file>
</module>
