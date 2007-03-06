<module name="icontest" type="win32gui" installbase="bin" installname="icontest.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<include base="icontest">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>ntdll</library>
	<file>icontest.c</file>
	<file>icontest.rc</file>
</module>
