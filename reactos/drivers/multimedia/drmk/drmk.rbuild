<module name="drmk" type="exportdriver" installbase="system32/drivers" installname="drmk.sys" allowwarnings="true">
	<include base="drmk">.</include>
	<include base="drmk">..</include>
	<include base="drmk">../include</include>
	<importlibrary definition="drmk.def" />
	<library>ntoskrnl</library>
	<define name="__USE_W32API" />
	<define name="BUILDING_DRMK" />
	<file>drmk.rc</file>
	<file>stubs.c</file>
</module>
