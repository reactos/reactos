<module name="mpu401_ks" type="exportdriver" installbase="system32/drivers" installname="mpu401_ks.sys" allowwarnings="true">
	<include base="mpu401">.</include>
	<include base="mpu401">..</include>
	<importlibrary definition="mpu401.def" />
	<library>ntoskrnl</library>
	<library>portcls</library>
	<define name="__USE_W32API" />
	<file>mpu401.rc</file>
	<file>adapter.cpp</file>
</module>
