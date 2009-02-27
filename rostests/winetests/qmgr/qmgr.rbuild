<module name="qmgr_winetest" type="win32cui" installbase="bin" installname="qmgr_winetest.exe" allowwarnings="true">
	<compilerflag compiler="cc">-Wno-format</compilerflag>
	<include base="qmgr_winetest">.</include>
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
