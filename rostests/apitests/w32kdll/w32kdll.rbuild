<module name="w32kdll" type="win32dll" entrypoint="0" installname="w32kdll.dll">
	<importlibrary definition="w32kdll.def" />
	<define name="__USE_W32API" />
	<define name="_WIN32_WINNT">0x0502</define>
	<library>win32ksys</library>
	<file>main.c</file>
</module>
