<module name="kbdlayout" type="win32gui" installbase="system32" installname="kbdlayout.exe">
	<include base="kbdlayout">.</include>
	<define name="__USE_W32API" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>kbdlayout.c</file>
	<file>kbdlayout.rc</file>
</module>
