<module name="miditest" type="win32gui" installbase="bin" installname="miditest.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="__USE_W32API" />
	<include base="miditest">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>miditest.c</file>
</module>
