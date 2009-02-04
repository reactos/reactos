<module name="telnetd" type="win32cui" installbase="system32" installname="telnetd.exe" allowwarnings="true" unicode="no">
	<include base="reactos"></include>
	<include base="telnetd">..</include>

	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>ws2_32</library>

	<file>telnetd.c</file>
	<file>serviceentry.c</file>
	<file>telnetd.rc</file>
</module>
