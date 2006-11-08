<module name="drawcap" type="win32cui" installbase="system32" installname="drawcap.exe">
	<include base="drawcap">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>drawcap.c</file>
	<file>drawcap.rc</file>
</module>

<module name="capicon" type="win32cui" installbase="system32" installname="capicon.exe">
	<include base="capicon">.</include>
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_WIN32_IE">0x0500</define>
	<define name="_WIN32_WINNT">0x0600</define>
	<define name="WINVER">0x0600</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>capicon.c</file>
	<file>capicon.rc</file>
</module>
