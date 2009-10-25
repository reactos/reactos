<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ramdisk" type="kernelmodedriver" installbase="system32/drivers" installname="ramdisk.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>ramdisk.c</file>
	<file>ramdisk.rc</file>
	<compilerflag compilerset="gcc">-fms-extensions</compilerflag>
</module>
