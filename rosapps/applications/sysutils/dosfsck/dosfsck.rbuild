<module name="dosfsck" type="win32cui" installbase="system32" installname="dosfsck.exe" unicode="no" allowwarnings="true">
	<library>kernel32</library>
	<library>user32</library>

	<file>boot.c</file>
	<file>check.c</file>
	<file>common.c</file>
	<file>dosfsck.c</file>
	<file>fat.c</file>
	<file>file.c</file>
	<file>io.c</file>
	<file>lfn.c</file>

	<file>dosfsck.rc</file>
</module>
