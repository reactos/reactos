<module name="chkdsk" type="win32cui" installbase="system32" installname="chkdsk.exe" allowwarnings="true" unicode="yes">
	<include base="reactos">include/reactos/libs/fmifs</include>
	<include base="chkdsk>..</include>

	<library>fmifs</library>
	<library>win32err</library>
	<library>ntdll</library>
	<library>kernel32</library>

	<file>chkdsk.c</file>
	<file>chkdsk.rc</file>
</module>
