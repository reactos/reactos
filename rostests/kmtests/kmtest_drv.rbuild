<module name="kmtest_drv" type="kernelmodedriver" installbase="bin" installname="kmtest_drv.sys">
	<include base="kmtest_drv">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>pseh</library>
	<library>kmtest_printf</library>
	<define name="KMT_KERNEL_MODE" />
	<define name="NTDDI_VERSION">NTDDI_WS03SP1</define>
	<directory name="kmtest_drv">
		<file>kmtest_drv.c</file>
		<file>testlist.c</file>
	</directory>
	<directory name="example">
		<file>Example.c</file>
		<file>GuardedMemory.c</file>
		<file>KernelType.c</file>
	</directory>
	<directory name="ntos_ex">
		<file>ExDoubleList.c</file>
		<file>ExFastMutex.c</file>
		<file>ExHardError.c</file>
		<file>ExInterlocked.c</file>
		<file>ExPools.c</file>
		<file>ExResource.c</file>
		<file>ExSequencedList.c</file>
		<file>ExSingleList.c</file>
		<file>ExTimer.c</file>
	</directory>
	<directory name="ntos_fsrtl">
		<file>FsRtlExpression.c</file>
	</directory>
	<directory name="ntos_io">
		<file>IoDeviceInterface.c</file>
		<file>IoInterrupt.c</file>
		<file>IoIrp.c</file>
		<file>IoMdl.c</file>
	</directory>
	<directory name="ntos_ke">
		<file>KeApc.c</file>
		<file>KeDpc.c</file>
		<file>KeEvent.c</file>
		<file>KeGuardedMutex.c</file>
		<file>KeIrql.c</file>
		<file>KeProcessor.c</file>
		<file>KeSpinLock.c</file>
		<file>KeTimer.c</file>
	</directory>
	<directory name="ntos_mm">
		<file>MmSection.c</file>
	</directory>
	<directory name="ntos_ob">
		<file>ObReference.c</file>
		<file>ObType.c</file>
	</directory>
	<directory name="rtl">
		<file>RtlAvlTree.c</file>
		<file>RtlMemory.c</file>
		<file>RtlSplayTree.c</file>
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
