<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="mstask_winetest" type="win32cui" installbase="bin" installname="mstask_winetest.exe" allowwarnings="true">
	<include base="mstask_winetest">.</include>
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>task.c</file>
	<file>task_scheduler.c</file>
	<file>task_trigger.c</file>
	<file>testlist.c</file>
	<library>wine</library>
	<library>ole32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
