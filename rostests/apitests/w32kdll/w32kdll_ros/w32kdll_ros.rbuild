<module name="w32kdll" type="win32dll" entrypoint="0" installname="w32kdll.dll">
	<importlibrary definition="w32kdll_ros.def" />
	<define name="__USE_W32API" />
	<library>win32ksys</library>
	<file>main.c</file>
</module>
