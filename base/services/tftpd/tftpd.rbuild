<module name="tftpd" type="win32cui" installbase="system32" installname="tftpd.exe" allowwarnings="true" unicode="no">
	<include base="reactos"></include>
	<include base="telnetd">..</include>

	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ws2_32</library>
	<library>wine</library>

	<file>tftpd.cpp</file>
</module>
