<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="dsound_winetest" type="win32cui" installbase="bin" installname="dsound_winetest.exe" allowwarnings="true">
	<include base="dsound_winetest">.</include>
    	<define name="__ROS_LONG64__" />
	<library>wine</library>
	<library>uuid</library>
	<library>dsound</library>
	<library>dxguid</library>
	<library>ole32</library>
	<library>user32</library>
	<file>capture.c</file>
	<file>ds3d8.c</file>
	<file>ds3d.c</file>
	<file>dsound8.c</file>
	<file>dsound.c</file>
	<file>duplex.c</file>
	<file>propset.c</file>
	<file>testlist.c</file>
</module>
</group>
