<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbstor" type="kernelmodedriver" installbase="system32/drivers" installname="usbstor.sys" allowwarnings="true">
	<importlibrary definition="usbstor.def" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>usbstor.c</file>
	<file>usbstor.rc</file>
</module>
