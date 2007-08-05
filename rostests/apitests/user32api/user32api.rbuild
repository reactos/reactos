<module name="user32api" type="win32cui">
	<include base="user32api">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>apitest</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<file>user32api.c</file>
	<file>testlist.c</file>
</module>
