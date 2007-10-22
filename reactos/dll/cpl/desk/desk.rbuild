<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="desk" type="win32dll" extension=".cpl" baseaddress="${BASEADDRESS_DESK}" installbase="system32" installname="desk.cpl" unicode="true" generatemanifest="true" generateresource="true">
	<importlibrary definition="desk.def" />
	<include base="desk">.</include>
	<include base="desk" root="intermediate">.</include>
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<define name="WINVER">0x501</define>
	<define name="_WIN32" />
	<library>kernel32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>gdi32</library>
	<library>comctl32</library>
	<library>comdlg32</library>
	<library>ole32</library>
	<library>setupapi</library>
	<library>shell32</library>
	<library>ntdll</library>
	<library>msimg32</library>
	<library>msvcrt</library>
	<library>uuid</library>
	<file>advmon.c</file>
	<file>appearance.c</file>
	<file>background.c</file>
	<file>classinst.c</file>
	<file>desk.c</file>
	<file>devsett.c</file>
	<file>dibitmap.c</file>
	<file>misc.c</file>
	<file>preview.c</file>
	<file>screensaver.c</file>
	<file>advappdlg.c</file>
	<file>settings.c</file>
	<file>monslctl.c</file>
	<file>desk.rc</file>

	<metadata description="ReactOS Display ControlPanel Applet" />

	<localization isoname="bg-BG">lang/bg-BG.rc</localization>
	<localization isoname="cs-CZ">lang/cs-CZ.rc</localization>
	<localization isoname="de-DE">lang/de-DE.rc</localization>
	<localization isoname="el-GR" dirty="true">lang/el-GR.rc</localization>
	<localization isoname="en-US">lang/en-US.rc</localization>
	<localization isoname="es-ES">lang/es-ES.rc</localization>
	<localization isoname="fr-FR">lang/fr-FR.rc</localization>
	<localization isoname="hu-HU">lang/hu-HU.rc</localization>
	<localization isoname="id-ID">lang/id-ID.rc</localization>
	<localization isoname="it-IT">lang/it-IT.rc</localization>
	<localization isoname="ja-JP">lang/ja-JP.rc</localization>
	<localization isoname="nl-NL">lang/nl-NL.rc</localization>
	<localization isoname="pl-PL" dirty="true">lang/pl-PL.rc</localization>
	<localization isoname="ru-RU">lang/ru-RU.rc</localization>
	<localization isoname="sk-SK">lang/sk-SK.rc</localization>
	<localization isoname="sv-SE">lang/sv-SE.rc</localization>
	<localization isoname="uk-UA">lang/uk-UA.rc</localization>
</module>
