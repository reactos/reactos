<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="pciide" type="kernelmodedriver" installbase="system32/drivers" installname="pciide.sys">
	<define name="__USE_W32API" />
	<library>pciidex</library>
	<library>ntoskrnl</library>
	<file>pciide.c</file>
	<file>pciide.rc</file>
</module>
</rbuild>
