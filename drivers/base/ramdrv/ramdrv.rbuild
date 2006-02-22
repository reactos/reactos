<module name="ramdrv" type="kernelmodedriver">
	<include base="ramdrv">.</include>
	<include base="bzip2">.</include>
        <define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>bzip2</library>
	<file>ramdrv.c</file>
	<file>ramdrv.rc</file>
</module>
