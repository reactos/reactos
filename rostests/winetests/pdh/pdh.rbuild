<module name="pdh_winetest" type="win32cui" installbase="bin" installname="pdh_winetest.exe" allowwarnings="true">
	<include base="pdh_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>pdh.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>pdh</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
