<module name="bootvid" type="kernelmodedll" entrypoint="DriverEntry@8" installbase="system32/drivers" installname="bootvid.dll">
	<importlibrary definition="bootvid.def"></importlibrary>
	<bootstrap base="$(CDOUTPUT)" nameoncd="bootvid.dll" />
	<include base="bootvid">.</include>
        <define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>bootvid.c</file>
	<file>vid.c</file>
	<file>vid_vga.c</file>
	<file>vid_vgatext.c</file>
	<file>vid_xbox.c</file>
	<file>bootvid.rc</file>
</module>
