<module name="wdmaud.drv" type="win32dll" baseaddress="${BASEADDRESS_WDMAUD}" installbase="system32" installname="wdmaud.drv" unicode="yes">
	<importlibrary definition="wdmaud.spec" />
	<include base="wdmaud.drv">.</include>
	<include base="ReactOS">include/reactos/libs/sound</include>
	<include base="wdmaud_kernel">.</include>
	<include base="mmixer">.</include>
	<include base="libsamplerate">.</include>
	<define name="NDEBUG">1</define>
	<!-- <define name="USE_MMIXER_LIB">1</define> Enable this line to bypass wdmaud + sysaudio -->
	<!-- <define name="USERMODE_MIXER">1</define> Enable this line to for usermode mixing support -->
	<library>mmebuddy</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>winmm</library>
	<library>advapi32</library>
	<library>libsamplerate</library>
	<library>msvcrt</library>
	<library>mmixer</library>
	<library>setupapi</library>
	<library>ksuser</library>
	<file>wdmaud.c</file>
	<file>mixer.c</file>
	<file>mmixer.c</file>
	<file>legacy.c</file>
	<file>wdmaud.rc</file>
</module>
