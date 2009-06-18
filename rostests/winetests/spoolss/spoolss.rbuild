<module name="spoolss_winetest" type="win32cui" installbase="bin" installname="spoolss_winetest.exe" allowwarnings="true">
	<include base="spoolss_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>spoolss.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
