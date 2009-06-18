<module name="wldap32_winetest" type="win32cui" installbase="bin" installname="wldap32_winetest.exe" allowwarnings="true">
	<include base="wldap32_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>parse.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>wldap32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
