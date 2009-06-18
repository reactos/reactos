<module name="msctf_winetest" type="win32cui" installbase="bin" installname="msctf_winetest.exe" allowwarnings="true">
	<include base="msctf_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>inputprocessor.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
