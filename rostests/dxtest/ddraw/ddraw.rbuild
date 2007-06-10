<module name="ddraw_test" type="win32cui" allowwarnings="true">
	<include base="ddraw_test">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ole32</library>
	<library>ddraw</library>
	<library>dxguid</library>
	<file>ddraw_test.cpp</file>
	<file>helper.cpp</file>
	<file>testlist.cpp</file>
</module>
