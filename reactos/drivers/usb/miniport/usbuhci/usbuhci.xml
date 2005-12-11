<module name="usbuhci" type="kernelmodedriver" installbase="system32/drivers" installname="usbuhci.sys">
	<define name="__USE_W32API" />
	<include>../linux</include>
	<include base="usbminiportcommon"></include>
	<library>sys_base</library>
	<library>usbminiportcommon</library>
	<library>usbport</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>uhci.c</file>
	<file>uhci-hcd.c</file>
	<file>uhci.rc</file>
</module>
