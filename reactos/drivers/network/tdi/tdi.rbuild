<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="tdi" type="kernelmodedriver" installbase="system32/drivers" installname="tdi.sys">
	<importlibrary definition="misc/tdi.def"></importlibrary>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<directory name="cte">
		<file>string.c</file>
		<file>stubs.c</file>
	</directory>
	<directory name="misc">
		<file>main.c</file>
		<file>tdi.rc</file>
	</directory>
	<directory name="tdi">
		<file>dereg.c</file>
		<file>handler.c</file>
		<file>obsolete.c</file>
		<file>stubs.c</file>
	</directory>
</module>
