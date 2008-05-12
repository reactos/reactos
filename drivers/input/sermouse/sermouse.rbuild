<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="sermouse" type="kernelmodedriver" installbase="system32/drivers" installname="sermouse.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>createclose.c</file>
	<file>detect.c</file>
	<file>fdo.c</file>
	<file>internaldevctl.c</file>
	<file>misc.c</file>
	<file>readmouse.c</file>
	<file>sermouse.c</file>
	<file>sermouse.rc</file>
</module>
