<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
<module name="pci" type="kernelmodedriver" installbase="system32/drivers" installname="pci.sys">
	<include base="pci">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>fdo.c</file>
	<file>pci.c</file>
	<file>pdo.c</file>
	<file>pci.rc</file>
</module>
</rbuild>
