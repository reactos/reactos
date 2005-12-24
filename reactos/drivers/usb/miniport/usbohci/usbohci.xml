<module name="usbohci" type="kernelmodedriver" installbase="system32/drivers" installname="usbohci.sys">
	<define name="__USE_W32API" />
	<include>../linux</include>
	<include base="usbminiportcommon"></include>
	<library>sys_base</library>
	<library>usbminiportcommon</library>
	<library>usbport</library>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>ohci.c</file>
	<file>ohci-hcd.c</file>
	<file>ohci.rc</file>
</module>
