<module name="sysaudio" type="kernelmodedriver" installbase="system32/drivers" installname="sysaudio.sys" allowwarnings="true">
	<include base="sysaudio">.</include>
	<library>ntoskrnl</library>
	<define name="__USE_W32API" />
	<define name="_NTDDK_" />
	<define name="_COMDDK_" />

	<file>main.c</file>

	<file>sysaudio.rc</file>
</module>
