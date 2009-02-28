<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="taskmgr" type="win32gui" installbase="system32" installname="taskmgr.exe" unicode="yes">
	<include base="taskmgr">.</include>
	<library>ntsys</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>comctl32</library>
	<pch>precomp.h</pch>
	<compilationunit name="unit.c">
		<file>about.c</file>
		<file>affinity.c</file>
		<file>applpage.c</file>
		<file>column.c</file>
		<file>dbgchnl.c</file>
		<file>debug.c</file>
		<file>endproc.c</file>
		<file>graph.c</file>
		<file>optnmenu.c</file>
		<file>perfdata.c</file>
		<file>perfpage.c</file>
		<file>priority.c</file>
		<file>procpage.c</file>
		<file>proclist.c</file>
		<file>run.c</file>
		<file>trayicon.c</file>
		<file>taskmgr.c</file>
		<file>graphctl.c</file>
	</compilationunit>
	<file>taskmgr.rc</file>
</module>
