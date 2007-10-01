<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="cdrom" type="kernelmodedriver" installbase="system32/drivers" installname="cdrom.sys" allowwarnings="true">
	<bootstrap installbase="$(CDOUTPUT)" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<library>scsiport</library>
	<include base="cdrom">..</include>
	<file>cdrom.c</file>
	<file>findscsi.c</file>
	<file>cdrom.rc</file>
</module>
