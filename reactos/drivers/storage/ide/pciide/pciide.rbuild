<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="pciide" type="kernelmodedriver" installbase="system32/drivers" installname="pciide.sys">
	<define name="__USE_W32API" />
	<library>pciidex</library>
	<library>ntoskrnl</library>
	<file>pciide.c</file>
	<file>pciide.rc</file>
</module>
