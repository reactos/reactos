<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="mountmgr" type="kernelmodedriver" installbase="system32/drivers" installname="mountmgr.sys" allowwarnings="true">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<define name="NTDDI_VERSION">0x05020400</define>
	<include base="mountmgr">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>ioevent</library>
	<library>wdmguid</library>
	<file>database.c</file>
	<file>device.c</file>
	<file>mountmgr.c</file>
	<file>notify.c</file>
	<file>point.c</file>
	<file>symlink.c</file>
	<file>uniqueid.c</file>
	<file>mountmgr.rc</file>
	<pch>mntmgr.h</pch>
</module>
