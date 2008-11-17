<module name="bitblt" type="win32gui" installbase="bin" installname="bitblt.exe">
	<define name="__USE_W32API" />
	<include base="bitblt">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>bitblt.c</file>
	<file>bitblt.rc</file>
</module>
