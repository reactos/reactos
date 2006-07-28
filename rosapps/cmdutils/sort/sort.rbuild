<module name="sort" type="win32cui" installbase="system32" installname="sort.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<file>sort.c</file>
	<file>sort.rc</file>
</module>
