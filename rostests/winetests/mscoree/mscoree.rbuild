<module name="mscoree_winetest" type="win32cui" installbase="bin" installname="mscoree_winetest.exe" allowwarnings="true">
	<include base="mscoree_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>debugging.c</file>
	<file>metahost.c</file>
	<file>mscoree.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>ntdll</library>
</module>
