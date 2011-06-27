<module name="kmtest_drv" type="kernelmodedriver" installbase="system32/drivers" installname="kmtest_drv.sys">
	<include base="kmtest_drv">include</include>
	<library>ntoskrnl</library>
	<library>ntdll</library>
	<library>hal</library>
	<library>pseh</library>
	<library>kmtest_printf</library>
	<define name="KMT_KERNEL_MODE" />
	<directory name="kmtest_drv">
		<file>kmtest_drv.c</file>
		<file>testlist.c</file>
	</directory>
	<directory name="example">
		<file>Example.c</file>
	</directory>
	<directory name="ntos_ex">
		<file>ExPools.c</file>
		<file>ExTimer.c</file>
	</directory>
	<directory name="ntos_fsrtl">
		<file>FsRtlExpression.c</file>
	</directory>
	<directory name="ntos_io">
		<file>IoDeviceInterface.c</file>
		<file>IoIrp.c</file>
		<file>IoMdl.c</file>
	</directory>
	<directory name="ntos_ke">
		<file>KeProcessor.c</file>
	</directory>
	<directory name="ntos_ob">
		<file>ObCreate.c</file>
	</directory>
</module>
<module name="kmtest_printf" type="staticlibrary">
	<include base="crt">include</include>
	<define name="_LIBCNT_" />
	<define name="_USER32_WSPRINTF" />
	<define name="wctomb">KmtWcToMb</define>
	<directory name="kmtest_drv">
		<file>printf_stubs.c</file>
	</directory>
	<directory name="..">
		<directory name="..">
			<directory name="..">
				<directory name="lib">
					<directory name="sdk">
						<directory name="crt">
							<directory name="printf">
								<file>streamout.c</file>
							</directory>
						</directory>
					</directory>
				</directory>
			</directory>
		</directory>
	</directory>
</module>
