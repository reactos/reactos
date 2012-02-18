<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbohci" type="kernelmodedriver" installbase="system32/drivers" installname="usbohci.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>usbd</library>
    <library>pseh</library>
	<file>usbohci.cpp</file>
	<file>usb_device.cpp</file>
	<file>usb_request.cpp</file>
	<file>usb_queue.cpp</file>
	<file>hcd_controller.cpp</file>
	<file>hardware.cpp</file>
	<file>misc.cpp</file>
	<file>purecall.cpp</file>
	<file>hub_controller.cpp</file>
	<file>memory_manager.cpp</file>
	<file>usbohci.rc</file>
	<compilerflag>-fno-rtti</compilerflag>
	<compilerflag>-fno-exceptions</compilerflag>
</module>
