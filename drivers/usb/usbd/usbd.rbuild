<module name="usbd" type="exportdriver" installbase="system32/drivers" installname="usbd.sys">
        <importlibrary definition="usbd.def" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>usbd.c</file>
	<file>usbd.rc</file>
</module>
