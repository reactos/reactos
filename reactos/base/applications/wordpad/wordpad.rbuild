<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="wordpad" type="win32gui" installbase="system32" installname="wordpad.exe" allowwarnings="true">
	<include base="wordpad">.</include>
    <define name="__ROS_LONG64__" />
	<library>comdlg32</library>
	<library>shell32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>comctl32</library>
	<file>print.c</file>
	<file>registry.c</file>
	<file>wordpad.c</file>
	<file>Da.rc</file>
	<file>De.rc</file>
	<file>En.rc</file>
	<file>Fr.rc</file>
	<file>Hu.rc</file>
	<file>Ja.rc</file>
	<file>Ko.rc</file>
	<file>Lt.rc</file>
	<file>Nl.rc</file>
	<file>No.rc</file>
	<file>Pl.rc</file>
	<file>Pt.rc</file>
	<file>Ru.rc</file>
	<file>Si.rc</file>
	<file>Sv.rc</file>
	<file>Tr.rc</file>
	<file>Zh.rc</file>
	<file>rsrc.rc</file>
</module>
