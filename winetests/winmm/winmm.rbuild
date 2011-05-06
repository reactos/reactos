<?xml version="1.0"?>
<module name="winmm_winetest" type="win32cui" installbase="bin" installname="winmm_winetest.exe" allowwarnings="true">
	<include base="winmm_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<library>dxguid</library>
	<library>winmm</library>
	<library>user32</library>
	<file>capture.c</file>
	<file>mci.c</file>
	<file>mixer.c</file>
	<file>mmio.c</file>
	<file>timer.c</file>
	<file>wave.c</file>
	<file>testlist.c</file>
</module>
