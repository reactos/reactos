<module name="example_drv" type="kernelmodedriver" installbase="system32/drivers" installname="example_drv.sys">
	<include base="kmtest_drv">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<library>kmtest_printf</library>
	<define name="KMT_STANDALONE_DRIVER" />
	<file>Example_drv.c</file>
	<directory name="..">
		<directory name="kmtest_drv">
			<file>kmtest_standalone.c</file>
		</directory>
	</directory>
</module>
