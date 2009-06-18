<module name="oleacc_winetest" type="win32cui" installbase="bin" installname="oleacc_winetest.exe" allowwarnings="true">
	<include base="oleacc_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>main.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>oleacc</library>
	<library>ntdll</library>
</module>
