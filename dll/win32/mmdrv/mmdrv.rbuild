<module name="mmdrv" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_MMDRV}" installbase="system32" installname="mmdrv.dll" unicode="yes">
	<importlibrary definition="mmdrv.spec" />
	<include base="mmdrv">.</include>
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
