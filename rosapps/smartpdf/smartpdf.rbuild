<module name="smartpdf" type="win32gui" installbase="system32" installname="smartpdf.exe" allowwarnings="true" stdlib="host">
 	<library>ntdll</library>
 	<library>kernel32</library>
 	<library>advapi32</library>
 	<library>comctl32</library>
 	<library>comdlg32</library>
 	<library>gdi32</library>
 	<library>msimg32</library>
 	<library>shell32</library>
 	<library>user32</library>
 	<library>winspool</library>
 	<library>fitz</library>
 	<library>poppler</library>
 	<library>freetype</library>
 	<library>libjpeg</library>
 	<library>zlib</library>
 	<define name="__USE_W32API" />
 	<define name="WIN32" />
 	<define name="_WIN32" />
 	<define name="_WINDOWS" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_DEBUG" />
	<define name="DEBUG" />
	<define name="USE_OWN_GET_AUTH_DATA" />
	<define name="POPPLER_DATADIR"></define>
	<define name="ENABLE_ZLIB">1</define>
	<define name="ENABLE_LIBJPEG">1</define>
	<define name="__REACTOS__" />
	<define name="WINVER">0x0500</define>
 	<include base="libjpeg">.</include>
 	<include base="zlib">.</include>
 	<include base="freetype">include</include>
 	<include base="freetype">include/freetype</include>
 	<include base="smartpdf">.</include>
 	<include base="fitz">.</include>
 	<include base="poppler">.</include>
 	<include>src</include>
 	<include>baseutils</include>
 	<include>fitz/include</include>
 	<include>poppler</include>
 	<include>poppler/goo</include>
 	<include>poppler/fofi</include>
 	<include>poppler/splash</include>
 	<include>poppler/poppler</include>
	<directory name="src>
	 	<file>AppPrefs.cc</file>
	 	<file>DisplayModel.cc</file>
	 	<file>DisplayModelFitz.cc</file>
	 	<file>DisplayModelSplash.cc</file>
	 	<file>DisplayState.cc</file>
	 	<file>FileHistory.cc</file>
	 	<file>PdfEngine.cc</file>
	 	<file>SumatraDialogs.cc</file>
	 	<file>SumatraPDF.cpp</file>
	 	<file>translations.cpp</file>
	 	<file>translations_txt.c</file>
	</directory>
	<directory name="baseutils>
	 	<file>base_util.c</file>
	 	<file>dstring.c</file>
	 	<file>file_util.c</file>
	 	<file>geom_util.c</file>
	 	<file>str_util.c</file>
	 	<file>win_util.c</file>
	</directory>
</module>
<!--
<module name="smartpdf" type="win32dll" entrypoint="0" baseaddress="0x10000000" installbase="system32" installname="smartpdf.dll" allowwarnings="true">
 	<importlibrary definition="smartpdf.def" />
 	<library>ntdll</library>
 	<library>kernel32</library>
 	<library>advapi32</library>
 	<library>comctl32</library>
 	<library>comdlg32</library>
 	<library>gdi32</library>
 	<library>msimg32</library>
 	<library>shell32</library>
 	<library>user32</library>
 	<library>winspool</library>
 	<library>libjpeg</library>
 	<library>zlib</library>
 	<library>freetype</library>
 	<define name="__USE_W32API" />
 	<define name="WIN32" />
 	<define name="_WIN32" />
 	<define name="_WINDOWS" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="_DEBUG" />
	<define name="DEBUG" />
	<define name="_DLL" />
	<define name="USE_OWN_GET_AUTH_DATA" />
	<define name="POPPLER_DATADIR"></define>
	<define name="ENABLE_ZLIB">1</define>
	<define name="ENABLE_LIBJPEG">1</define>
	<define name="__REACTOS__" />
 	<include base="libjpeg">.</include>
 	<include base="zlib">.</include>
 	<include base="freetype">include</include>
 	<include base="smartpdf">.</include>
 	<include base="fitz">.</include>
 	<include base="poppler">.</include>
 	<include>src</include>
 	<include>baseutils</include>
 	<include>fitz/include</include>
 	<include>poppler</include>
 	<include>poppler/goo</include>
 	<include>poppler/fofi</include>
 	<include>poppler/splash</include>
 	<include>poppler/poppler</include>
	<directory name="src>
	 	<file>AppPrefs.cc</file>
	 	<file>DisplayModel.cc</file>
	 	<file>DisplayModelFitz.cc</file>
	 	<file>DisplayModelSplash.cc</file>
	 	<file>DisplayState.cc</file>
	 	<file>FileHistory.cc</file>
	 	<file>PdfEngine.cc</file>
	 	<file>SumatraDialogs.cc</file>
	 	<file>SumatraPDF.cpp</file>
	 	<file>translations.cpp</file>
	 	<file>translations_txt.c</file>
	</directory>
	<directory name="baseutils>
	 	<file>base_util.c</file>
	 	<file>dstring.c</file>
	 	<file>file_util.c</file>
	 	<file>geom_util.c</file>
	 	<file>str_util.c</file>
	 	<file>win_util.c</file>
	</directory>
</module>
-->
