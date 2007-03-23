<module name="ddraw_demo" type="win32gui" installbase="system32" installname="ddraw_demo.exe" allowwarnings="true">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<library>ddraw</library>
	<library>dxguid</library>
	<library>user32</library>
	<file>main.cpp</file>
</module>