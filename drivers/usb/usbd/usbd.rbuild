<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="usbd" type="kernelmodedriver" installbase="system32/drivers" installname="usbd.sys">
        <importlibrary definition="usbd.def" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>usbd.c</file>
	<file>usbd.rc</file>
</module>
</rbuild>
