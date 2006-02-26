<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="beep" type="kernelmodedriver" installbase="system32/drivers" installname="beep.sys">
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>beep.c</file>
	<file>beep.rc</file>
</module>
</rbuild>
