<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="GUI" type="win32gui" installbase="system32" installname="testGUI.exe" allowwarnings="true" unicode="yes">
	<include base="GUI">.</include>
	<define name="_WIN32_IE">0x600</define>
	<redefine name="_WIN32_WINNT">0x501</redefine>
    <define name="__ROS_LONG64__" />
	<library>gdi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>browsewnd.c</file>
	<file>mainwnd.c</file>
	<file>misc.c</file>
	<file>WinetestsGUI.rc</file>
	<pch>precomp.h</pch>
</module>
