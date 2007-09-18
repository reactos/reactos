<module name="audiosrv" type="win32cui" installbase="system32" installname="audiosrv.exe" allowwarnings="true">
	<include base="audiosrv">.</include>
	<define name="__USE_W32API" />
	<define name="__REACTOS__" />
	<define name="_WIN32_WINNT">0x0501</define>
	<library>kernel32</library>
	<library>advapi32</library>
	<file>main.c</file>
	<file>audiosrv.rc</file>
</module>
