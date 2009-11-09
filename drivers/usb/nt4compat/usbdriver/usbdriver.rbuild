<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="usbdriver" type="kernelmodedriver" installbase="system32/drivers" installname="usbdriver.sys">
	<define name="INCLUDE_EHCI" />
	<define name="_X86" />
	<include base="usbdriver">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>ehci.c</file>
	<file>ohci.c</file>
	<file>uhci.c</file>
	<file>roothub.c</file>
	<file>hub.c</file>
	<file>td.c</file>
	<file>usb.c</file>
	<file>umss.c</file>
	<file>bulkonly.c</file>
	<file>cbi.c</file>
	<file>devmgr.c</file>
	<file>dmgrdisp.c</file>
	<file>compdrv.c</file>
	<file>etd.c</file>
	<file>gendrv.c</file>
	<file>mouse.c</file>
	<file>keyboard.c</file>
	<file>usbdriver.rc</file>
	<pch>usbdriver.h</pch>
</module>
