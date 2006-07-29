<module name="chkdsk" type="win32cui" installbase="system32" installname="chkdsk.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>fmifs</library>
	<library>ntdll</library>
	<file>chkdsk.c</file>
	<file>chkdsk.rc</file>
</module>
