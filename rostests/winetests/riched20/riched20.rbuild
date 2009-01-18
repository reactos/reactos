<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="riched20_winetest" type="win32cui" installbase="bin" installname="riched20_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="riched20_winetest">.</include>
    <define name="__ROS_LONG64__" />
	<file>editor.c</file>
	<file>richole.c</file>
	<file>testlist.c</file>
	<file>txtsrv.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
