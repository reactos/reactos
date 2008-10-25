<module name="package" type="win32dll" installbase="system32" installname="package.dll" allowwarnings="true" stdlib="host">
	<importlibrary definition="package.def" />
	<include base="package">.</include>

	<define name="UNICODE" />

	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<library>expat</library>
	<library>urlmon</library>
	<library>shell32</library>

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
