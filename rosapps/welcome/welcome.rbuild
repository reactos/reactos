<module name="rosapps_welcome" type="win32cui" installname="welcome.exe">
	<include base="rosapps_welcome">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
 	<file>welcome.c</file>
	<file>welcome.rc</file>
</module>

<module name="autorun" type="win32cui" installname="autorun.exe">
	<include base="autorun">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>welcome.c</file>
	<file>welcome.rc</file>
</module>
