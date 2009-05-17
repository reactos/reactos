<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="comctl32_winetest" type="win32cui" installbase="bin" installname="comctl32_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="comctl32_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>comboex.c</file>
	<file>datetime.c</file>
	<file>dpa.c</file>
	<file>header.c</file>
	<file>imagelist.c</file>
	<file>ipaddress.c</file>
	<file>listview.c</file>
	<file>misc.c</file>
	<file>monthcal.c</file>
	<file>mru.c</file>
	<file>msg.c</file>
	<file>progress.c</file>
	<file>propsheet.c</file>
	<file>rebar.c</file>
	<file>status.c</file>
	<file>subclass.c</file>
	<file>tab.c</file>
	<file>toolbar.c</file>
	<file>tooltips.c</file>
	<file>trackbar.c</file>
	<file>treeview.c</file>
	<file>updown.c</file>
	<file>rsrc.rc</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>comctl32</library>
	<library>ole32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
