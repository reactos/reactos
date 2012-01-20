<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="cdrom_new" type="kernelmodedriver" installbase="system32/drivers" installname="cdrom_new.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>classpnp</library>
	<include base="cdrom_new">../inc</include>
	<group compilerset="gcc">
		<compilerflag>-mrtd</compilerflag>
		<compilerflag>-fno-builtin</compilerflag>
		<compilerflag>-w</compilerflag>
	</group>
	<file>cdrom.c</file>
	<file>data.c</file>
	<file>ioctl.c</file>
	<file>mmc.c</file>
	<file>scsicdrm.rc</file>
	<file>sec.c</file>
</module>
