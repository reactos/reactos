<module name="usbport" type="kernelmodedriver" installbase="system32/drivers" installname="usbport.sys">
	<importlibrary definition="usbport.def" />
	<library>sys_base</library> 
	<library>ntoskrnl</library>
	<library>hal</library> 
	<file>message.c</file>
	<file>hcd.c</file>
	<file>hcd-pci.c</file>
	<file>hub.c</file>
	<file>usb.c</file>
	<file>config.c</file>
	<file>urb.c</file>
	<file>buffer_simple.c</file>
	<file>usb-debug.c</file>
	<file>usbcore.c</file>
	<directory name="core_drivers">
		<file>usbkey.c</file>
		<file>usbmouse.c</file>
	</directory>

	<file>usbcore.rc</file>
</module>
