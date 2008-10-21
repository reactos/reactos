<module name="cards" type="win32dll" baseaddress="${BASEADDRESS_CARDS}" installbase="system32" installname="cards.dll" unicode="yes">
	<importlibrary definition="cards.spec" />
	<include base="cards">.</include>

	<!-- Possible definitions: CARDSTYLE_DEFAULT or CARDSTYLE_BAVARIAN -->
	<define name="CARDSTYLE_DEFAULT" />

	<library>kernel32</library>
	<library>gdi32</library>
	<library>user32</library>
	<file>cards.c</file>
	<file>cards.rc</file>
	<file>cards.spec</file>
</module>
