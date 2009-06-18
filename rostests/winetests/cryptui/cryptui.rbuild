<module name="cryptui_winetest" type="win32cui" installbase="bin" installname="cryptui_winetest.exe" allowwarnings="true">
	<include base="cryptui_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>cryptui.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>cryptui</library>
	<library>crypt32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>ntdll</library>
</module>