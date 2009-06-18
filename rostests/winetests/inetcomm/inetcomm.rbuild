<module name="inetcomm_winetest" type="win32cui" installbase="bin" installname="inetcomm_winetest.exe" allowwarnings="true">
	<include base="inetcomm_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>mimeintl.c</file>
	<file>mimeole.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>inetcomm</library>
	<library>oleaut32</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
