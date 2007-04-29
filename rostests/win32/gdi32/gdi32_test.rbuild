<module name="gdi32_test" type="win32cui">
	<include base="gdi32_test">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>gdi32_test.c</file>
	<file>testlist.c</file>
</module>
