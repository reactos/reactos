<module name="lsdd" type="win32cui" installbase="system32" installname="lsdd.exe">
	<linkerflag>--numeric-sort</linkerflag>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>lib</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>lsdd.c</file>
	<file>lsdd.rc</file>
</module>
