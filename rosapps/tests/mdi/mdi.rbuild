<module name="mdi" type="win32gui" installbase="bin" installname="mdi.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<include base="mdi">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<file>mdi.c</file>
	<file>mdi.rc</file>
</module>
