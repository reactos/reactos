<?xml version="1.0"?>
<module name="eventvwr" type="win32gui" installbase="system32" installname="eventvwr.exe" generatemanifest="true" generateresource="true">
	<include base="eventvwr">.</include>
	<include base="eventvwr" root="intermediate">.</include>
	<define name="__REACTOS__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x501</define>
	<library>kernel32</library>
	<library>user32</library>
	<library>comctl32</library>
	<library>advapi32</library>
	<file>eventvwr.c</file>
	<file>eventvwr.rc</file>

	<metadata description="ReactOS Event Log Viewer" />

	<localization isoname="de-DE">lang/de-DE.rc</localization>
	<localization isoname="en-US">lang/en-US.rc</localization>
	<localization isoname="es-ES">lang/es-ES.rc</localization>
	<localization isoname="fr-FR">lang/fr-FR.rc</localization>
	<localization isoname="ru-RU">lang/ru-RU.rc</localization>
</module>

