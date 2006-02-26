<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="portcls" type="kernelmodedriver" installbase="system32/drivers" installname="portcls.sys">
        <importlibrary definition="portcls.def" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>portcls.c</file>
	<file>portcls.rc</file>
</module>
</rbuild>
