<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="usbstor" type="kernelmodedriver" installbase="system32/drivers" installname="usbstor.sys" allowwarnings="true">
	<define name="__USE_W32API" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>usbstor.c</file>
	<file>usbstor.rc</file>
</module>
</rbuild>
