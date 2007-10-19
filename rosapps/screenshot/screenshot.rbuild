<module name="screenshot" type="win32gui" installbase="system32" installname="screenshot.exe" unicode="yes">
	<include base="screenshot">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>comdlg32</library>
	<file>screenshot.c</file>
	<file>screenshot.rc</file>
</module>
