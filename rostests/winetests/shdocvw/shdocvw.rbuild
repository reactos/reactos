<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="shdocvw_winetest" type="win32cui" installbase="bin" installname="shdocvw_winetest.exe" allowwarnings="true">
	<include base="shdocvw_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>intshcut.c</file>
	<file>shdocvw.c</file>
	<file>shortcut.c</file>
	<file>webbrowser.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
