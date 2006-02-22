<module name="csr" type="nativecui" installbase="system32" installname="csr.exe">
	<include base="csr">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0501</define>
	<library>nt</library>
	<library>ntdll</library>
	<library>csrsrv</library>
	<file>main.c</file>
</module>
<directory name="csrsrv">
	<xi:include href="csrsrv/csrsrv.rbuild" />
</directory>
