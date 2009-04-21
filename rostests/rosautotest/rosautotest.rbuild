<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rosautotest" type="win32cui" installbase="system32" installname="rosautotest.exe" unicode="yes">
	<compilerflag compiler="cxx">-fno-rtti</compilerflag>
	<include base="rosautotest">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>shell32</library>
	<library>user32</library>
	<library>wininet</library>
	<file>CConfiguration.cpp</file>
	<file>CFatalException.cpp</file>
	<file>CInvalidParameterException.cpp</file>
	<file>CJournaledTestList.cpp</file>
	<file>CProcess.cpp</file>
	<file>CSimpleException.cpp</file>
	<file>CTest.cpp</file>
	<file>CTestInfo.cpp</file>
	<file>CTestList.cpp</file>
	<file>CVirtualTestList.cpp</file>
	<file>CWebService.cpp</file>
	<file>CWineTest.cpp</file>
	<file>main.cpp</file>
	<file>shutdown.cpp</file>
	<file>tools.cpp</file>
	<pch>precomp.h</pch>
</module>
