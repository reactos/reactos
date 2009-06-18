<module name="inetmib1_winetest" type="win32cui" installbase="bin" installname="inetmib1_winetest.exe" allowwarnings="true">
	<include base="inetmib1_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>main.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>snmpapi</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
