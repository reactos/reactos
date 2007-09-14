<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="beep" type="kernelmodedriver" installbase="system32/drivers" installname="beep.sys">
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>beep.c</file>
	<file>beep.rc</file>
</module>
