<module name="alphablend" type="win32gui" installbase="bin" installname="alphablend.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<include base="alphablend">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>alphablend.c</file>
	<file>alphablend.rc</file>
</module>
