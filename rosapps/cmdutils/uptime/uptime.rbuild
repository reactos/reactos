<module name="uptime" type="win32cui" installbase="system32" installname="uptime.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
<!-- <define name="LINUX_OUTPUT"></define> -->
	<library>kernel32</library>
	<file>uptime.c</file>
	<file>uptime.rc</file>
</module>
