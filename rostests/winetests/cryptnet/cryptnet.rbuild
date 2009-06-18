<module name="cryptnet_winetest" type="win32cui" installbase="bin" installname="cryptnet_winetest.exe" allowwarnings="true">
	<include base="cryptnet_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>cryptnet.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>cryptnet</library>
	<library>crypt32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
