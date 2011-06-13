<module name="kmtest_drv" type="kernelmodedriver" installbase="system32/drivers" installname="kmtest_drv.sys">
	<include base="kmtest_drv">include</include>
	<library>ntoskrnl</library>
	<library>ntdll</library>
	<library>hal</library>
	<library>pseh</library>
	<directory name="kmtest_drv">
		<file>kmtest_drv.c</file>
		<file>log.c</file>
		<file>testlist.c</file>
	</directory>
	<directory name="example">
		<file>Example.c</file>
	</directory>
</module>
