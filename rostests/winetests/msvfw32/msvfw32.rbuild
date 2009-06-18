<module name="msvfw32_winetest" type="win32cui" installbase="bin" installname="msvfw32_winetest.exe" allowwarnings="true">
	<include base="msvfw32_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>msvfw.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>msvfw32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
