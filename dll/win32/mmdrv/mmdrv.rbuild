<module name="mmdrv" type="win32dll" baseaddress="${BASEADDRESS_MMDRV}" installbase="system32" installname="mmdrv.dll">
	<importlibrary definition="mmdrv.def" />
	<include base="mmdrv">.</include>
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="NDEBUG" />
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>entry.c</file>
	<file>mme.c</file>
	<file>kernel.c</file>
	<file>session.c</file>
	<file>common.c</file>
	<file>wave.c</file>
	<file>wave_io.c</file>
</module>
