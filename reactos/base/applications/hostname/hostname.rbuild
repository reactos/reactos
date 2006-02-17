<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="hostname" type="win32cui" installbase="system32" installname="hostname.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<file>hostname.c</file>
	<file>hostname.rc</file>
</module>

