<module name="userenv_winetest" type="win32cui" installbase="bin" installname="userenv_winetest.exe" allowwarnings="true">
	<include base="userenv_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>userenv.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>userenv</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
