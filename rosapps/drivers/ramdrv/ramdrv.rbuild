<module name="ramdrv" type="kernelmodedriver">
	<include base="ramdrv">.</include>
	<include base="bzip2">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>bzip2</library>
	<file>ramdrv.c</file>
	<file>ramdrv.rc</file>
</module>
