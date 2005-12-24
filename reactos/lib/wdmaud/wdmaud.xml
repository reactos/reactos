<module name="wdmaud" type="win32dll" extension=".drv" baseaddress="${BASEADDRESS_WDMAUD}" installbase="system32" installname="wdmaud.drv">
	<importlibrary definition="wdmaud.def" />
	<include base="wdmaud">.</include>
	<define name="__USE_W32API" />
	<define name="_DISABLE_TIDENTS" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="__REACTOS__" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>winmm</library>
	<library>setupapi</library>
	<file>user.c</file>
	<file>kernel.c</file>
	<file>devices.c</file>
    <file>midi.c</file>
	<file>wave.c</file>
	<file>threads.c</file>
	<file>helper.c</file>
	<file>memtrack.c</file>
	<file>wdmaud.rc</file>
</module>
