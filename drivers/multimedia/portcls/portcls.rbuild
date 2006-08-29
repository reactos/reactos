<module name="portcls" type="exportdriver" installbase="system32/drivers" installname="portcls.sys">
        <importlibrary definition="portcls.def" />
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>portcls.c</file>
	<file>portcls.rc</file>
</module>
