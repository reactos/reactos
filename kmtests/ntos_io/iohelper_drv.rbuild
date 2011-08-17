<module name="iohelper_drv" type="kernelmodedriver" installbase="bin" installname="iohelper_drv.sys">
	<include base="kmtest_drv">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<library>kmtest_printf</library>
	<define name="KMT_STANDALONE_DRIVER" />
	<file>IoHelper_drv.c</file>
	<directory name="..">
		<directory name="kmtest_drv">
			<file>kmtest_standalone.c</file>
		</directory>
	</directory>
</module>
