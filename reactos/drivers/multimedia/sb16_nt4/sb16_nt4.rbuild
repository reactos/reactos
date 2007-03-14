<module name="sb16_nt4" type="exportdriver" installbase="system32/drivers" installname="sndblst.sys" allowwarnings="true">
	<include base="sb16_nt4">.</include>
	<include base="sb16_nt4">..</include>
	<importlibrary definition="sb16_nt4.def" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<define name="__USE_W32API" />
	<file>main.c</file>
	<file>control.c</file>
	<file>interrupt.c</file>
</module>
