<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="serial" type="kernelmodedriver" installbase="system32/drivers" installname="serial.sys">
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>circularbuffer.c</file>
	<file>cleanup.c</file>
	<file>close.c</file>
	<file>create.c</file>
	<file>devctrl.c</file>
	<file>info.c</file>
	<file>legacy.c</file>
	<file>misc.c</file>
	<file>pnp.c</file>
	<file>power.c</file>
	<file>rw.c</file>
	<file>serial.c</file>
	<file>serial.rc</file>
</module>
</rbuild>
