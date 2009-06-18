<module name="ntprint_winetest" type="win32cui" installbase="bin" installname="ntprint_winetest.exe" allowwarnings="true">
	<include base="ntprint_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>ntprint.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
