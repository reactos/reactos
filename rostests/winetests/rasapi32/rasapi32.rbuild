<module name="rasapi32_winetest" type="win32cui" installbase="bin" installname="rasapi32_winetest.exe" allowwarnings="true">
	<include base="rasapi32_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>rasapi.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
