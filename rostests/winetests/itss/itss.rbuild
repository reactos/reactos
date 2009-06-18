<module name="itss_winetest" type="win32cui" installbase="bin" installname="itss_winetest.exe" allowwarnings="true">
	<include base="itss_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>protocol.c</file>
	<file>rsrc.rc</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
