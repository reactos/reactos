<module name="qmgr_winetest" type="win32cui" installbase="bin" installname="qmgr_winetest.exe" allowwarnings="true">
	<include base="qmgr_winetest">.</include>
	<define name="__ROS_LONG64__" />
	<file>enum_files.c</file>
	<file>enum_jobs.c</file>
	<file>file.c</file>
	<file>job.c</file>
	<file>qmgr.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>shlwapi</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
