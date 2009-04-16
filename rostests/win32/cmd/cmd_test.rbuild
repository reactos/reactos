<module name="cmd_test" type="test">
	<include base="rtshared">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="cmd">.</include>
	<define name="ANONYMOUSUNIONS" />
	<redefine name="_WIN32_WINNT">0x501</redefine>
	<library>rtshared</library>
	<library>regtests</library>
	<library>cmd_base</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>cmd_base</library>
	<library>pseh</library>
	<library>ntdll</library>
	<file>setup.c</file>
	<xi:include href="stubs.rbuild" />
</module>
