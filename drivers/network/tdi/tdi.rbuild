<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="tdi" type="kernelmodedriver" installbase="system32/drivers" installname="tdi.sys">
	<importlibrary definition="misc/tdi.spec"></importlibrary>
	<library>ntoskrnl</library>
	<library>hal</library>
	<define name="_TDI_" />
	<directory name="cte">
		<file>events.c</file>
		<file>string.c</file>
		<file>timer.c</file>
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
