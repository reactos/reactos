<module name="unicows" type="win32dll" baseaddress="${BASEADDRESS_SFC}" installbase="system32" installname="unicows.dll" allowwarnings="yes">
	<importlibrary definition="unicows.spec" />
	<library>avicap32</library>
	<library>msvfw32</library>
	<library>oledlg</library>
	<library>comdlg32</library>
	<library>shell32</library>
	<library>winmm</library>
	<library>winspool</library>
	<library>rasapi32</library>
	<library>secur32</library>
	<library>sensapi</library>
	<library>version</library>
	<library>user32</library>
	<library>mpr</library>
	<library>gdi32</library>
	<library>advapi32</library>
</module>
