<module name="snmpapi_winetest" type="win32cui" installbase="bin" installname="snmpapi_winetest.exe" allowwarnings="true">
	<include base="snmpapi_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>util.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>snmpapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
