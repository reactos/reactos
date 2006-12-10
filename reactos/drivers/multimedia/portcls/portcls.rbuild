<module name="portcls" type="exportdriver" installbase="system32/drivers" installname="portcls.sys" allowwarnings="true">
        <importlibrary definition="portcls.def" />
	<define name="__USE_W32API" />
	<include base="portcls">../include</include>
	<library>ntoskrnl</library>
	<library>drmk</library>
    <file>dll.c</file>
	<file>adapter.c</file>
	<file>drm.c</file>
	<file>stubs.c</file>
	<file>portcls.rc</file>
</module>
