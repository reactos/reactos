<module name="msvcrtd_winetest" type="win32cui" installbase="bin" installname="msvcrtd_winetest.exe" allowwarnings="true">
	<include base="msvcrtd_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>debug.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
