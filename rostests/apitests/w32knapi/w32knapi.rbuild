<module name="w32knapi" type="win32cui">
	<include base="w32knapi">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0501</define>
	<define name="WINVER">0x501</define>
	<library>apitest</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>shell32</library>
	<library>w32kdll</library>
	<file>w32knapi.c</file>
	<file>testlist.c</file>
	<file>w32knapi.rc</file>
</module>
