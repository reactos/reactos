<module name="localui_winetest" type="win32cui" installbase="bin" installname="localui_winetest.exe" allowwarnings="true">
	<include base="localui_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>localui.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>winspool</library>
	<library>ntdll</library>
</module>
