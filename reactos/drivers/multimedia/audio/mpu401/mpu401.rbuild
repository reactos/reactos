<module name="mpu401" type="kernelmodedriver">
	<include base="mpu401">.</include>
        <define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>mpu401.c</file>
	<file>portio.c</file>
	<file>settings.c</file>
	<file>mpu401.rc</file>
</module>
