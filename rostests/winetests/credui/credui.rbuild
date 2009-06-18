<module name="credui_winetest" type="win32cui" installbase="bin" installname="credui_winetest.exe" allowwarnings="true">
	<include base="credui_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>credui.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>credui</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
