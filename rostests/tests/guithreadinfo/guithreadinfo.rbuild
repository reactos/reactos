<module name="guithreadinfo" type="win32gui" installbase="bin" installname="guithreadinfo.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>gdi32</library>
	<file>guithreadinfo.c</file>
</module>
