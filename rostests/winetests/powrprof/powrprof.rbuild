<module name="powrprof_winetest" type="win32cui" installbase="bin" installname="powrprof_winetest.exe" allowwarnings="true">
	<include base="powrprof_winetest">.</include>
    <define name="__ROS_LONG64__" />
    <define name="UNICODE" />
    <define name="_UNICODE" />
	<library>powrprof</library>
	<library>ntdll</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<file>testlist.c</file>
	<file>pwrprof.c</file>
</module>
