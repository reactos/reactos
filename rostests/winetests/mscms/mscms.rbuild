<module name="mscms_winetest" type="win32cui" installbase="bin" installname="mscms_winetest.exe" allowwarnings="true">
	<include base="mscms_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>profile.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
