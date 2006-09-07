<module name="package" type="win32dll" installbase="system32" installname="package.dll" allowwarnings="true" stdlib="host">
	<importlibrary definition="package.def" />
	<include base="package">.</include>

	<define name="UNICODE" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>

	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>expat</library>
	<library>urlmon</library>

	<file>download.cpp</file>
	<file>functions.cpp</file>
	<file>log.cpp</file>
	<file>main.cpp</file>
	<file>options.cpp</file>
	<file>package.cpp</file>
	<file>script.cpp</file>
	<file>tree.cpp</file>
	<file>package.rc</file>
</module>
