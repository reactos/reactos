<module name="win32kdxtest" type="win32cui" installbase="bin" installname="win32kdxtest.exe" allowwarnings ="true" >
	<include base="win32kdxtest">.</include>
	<define name="__USE_W32API" />
	<define name="__REACTOS__" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>main.c</file>
	<file>dump.c</file>
     	<file>../../../../dll/win32/gdi32/misc/win32k.S</file>

	
</module>