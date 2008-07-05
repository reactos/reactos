<module name="chklib" type="win32cui" installbase="system32" installname="chklib.exe" allowwarnings="true">
	<linkerflag>--numeric-sort</linkerflag>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>win32err</library>
	<file>chklib.c</file>
	<file>chklib.rc</file>
</module>
