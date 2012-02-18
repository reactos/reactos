<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbstor" type="kernelmodedriver" installbase="system32/drivers" installname="usbstor.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>usbd</library>
	<file>descriptor.c</file>
	<file>disk.c</file>
	<file>fdo.c</file>
	<file>misc.c</file>
	<file>pdo.c</file>
	<file>queue.c</file>
	<file>error.c</file>
	<file>scsi.c</file>
	<file>usbstor.c</file>
	<file>usbstor.rc</file>
</module>
