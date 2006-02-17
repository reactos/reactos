<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="devmgrapp" type="win32cui" installbase="system32" installname="devmgr.exe">
	<define name="__USE_W32API" />
	<define name="DEFINE_GUID" />
	<library>ntdll</library>
	<library>setupapi</library>
	<library>gdi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<file>devmgr.c</file>
	<efile>devmgr.rc</efile>
</module>
