@echo off

rem
rem         !!!!!!  dont forget that there MUST be a property usage too !!!!!!
rem

set l_SAUTH=1.3.6.1.5.5.7.3.1
set l_CAUTH=1.3.6.1.5.5.7.3.2
set l_CSIGN=1.3.6.1.5.5.7.3.3
set l_EMAIL=1.3.6.1.5.5.7.3.4
set l_TSTMP=1.3.6.1.5.5.7.3.8
set l_SVRGT=1.3.6.1.4.1.311.10.3.3
set l_NETSC=2.16.840.1.113730.4.1

set l_DISABLE=1.3.6.1.4.1.311.10.4.1

set l_CMGR=certmgr -add -all -c 

echo .
echo . checking out *.sst
echo .

out *.sst

if exist roots.sst    del roots.sst
if exist cas.sst      del cas.sst



rem --------------------------------------------------------------------------------------------------------------
rem         ***     VERISIGN     ***
rem --------------------------------------------------------------------------------------------------------------

set l_NAME=VeriSign Commercial Software Publishers CA
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\mscom1999.509      roots.sst
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\mscom2004.509      roots.sst

set l_NAME=VeriSign Individual Software Publishers CA
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\msind1999.509      roots.sst
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\msind2004.509      roots.sst

set l_NAME=VeriSign Class 1 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\class1-v0.509      roots.sst
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\class1-v1.509      roots.sst
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\class1-v2.509      roots.sst

set l_NAME=VeriSign Class 2 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\class2-v1.509      roots.sst
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\class2-v2.509      roots.sst

set l_NAME=VeriSign Class 3 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\class3-v1.509      roots.sst
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\class3-v2.509      roots.sst

set l_NAME=VeriSign Class 4 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\class4-v1.509      roots.sst

set l_NAME=VeriSign/RSA Commercial CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              rsa\rsa-cca.crt             roots.sst

set l_NAME=VeriSign/RSA Secure Server CA
%l_CMGR% -eku "%l_SAUTH%" -name "%l_NAME%"                                  rsa\rsa-ssca.crt            roots.sst
%l_CMGR% -eku "%l_SAUTH%" -name "%l_NAME%"                                  rsa\sscav2.509              roots.sst

rem ------      this is the "us" cert -- we don't want to ship this!
rem ------ set l_NAME=VeriSign Online Revocation Status Service
rem ------ %l_CMGR% -name "%l_NAME%"                                        verisign\crlsign-v1.509     roots.sst

set l_NAME=VeriSign Time Stamping CA
%l_CMGR% -eku "%l_TSTMP%" -name "%l_NAME%"                                  verisign\timeroot.509       roots.sst


%l_CMGR%                                                                    verisign\class1iv1.509      cas.sst

%l_CMGR%                                                                    verisign\class2iv1.509      cas.sst


rem --------------------------------------------------------------------------------------------------------------
rem         ***     MICROSOFT     ***
rem --------------------------------------------------------------------------------------------------------------

set l_NAME=Microsoft Authenticode(tm) Root
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        msft\msroot99.cer           roots.sst

set l_NAME=Microsoft Timestamp Root
%l_CMGR% -eku "%l_TSTMP%" -name "%l_NAME%"                                  msft\hawking.cer            roots.sst

set l_NAME=Microsoft Root SGC Authority
%l_CMGR% -eku "%l_SAUTH%,%l_SVRGT%,%l_NETSC%" -name "%l_NAME%"              msft\sgcroot.crt            roots.sst

%l_CMGR%                                                                    msft\whqlroot.cer           roots.sst


%l_CMGR% -eku "%l_SAUTH%,%l_SVRGT%,%l_NETSC%"                               msft\sgc_ca.crt             cas.sst

%l_CMGR%                                                                    msft\mstemp.cer             cas.sst

%l_CMGR%                                                                    test\mstest.cer             cas.sst


rem --------------------------------------------------------------------------------------------------------------
rem         ***     GTE     ***
rem --------------------------------------------------------------------------------------------------------------

set l_NAME=GTE CyberTrust Root
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              gte\ct_root.cer             roots.sst

rem --------------------------------------------------------------------------------------------------------------
rem         ***     ATT     ***
rem --------------------------------------------------------------------------------------------------------------

set l_NAME=ATT Certificate Services
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              att\att.crt                 roots.sst

set l_NAME=ATT Directory Services
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              att\attdir.crt              roots.sst

rem --------------------------------------------------------------------------------------------------------------
rem         ***     THAWTE     ***
rem --------------------------------------------------------------------------------------------------------------

%l_CMGR% -eku "%l_CAUTH%,%l_EMAIL%,%l_CSIGN%"                               thawte\pbca2020.crt         roots.sst
%l_CMGR% -eku "%l_CAUTH%,%l_EMAIL%,%l_CSIGN%"                               thawte\ppca2020.crt         roots.sst
%l_CMGR% -eku "%l_CAUTH%,%l_EMAIL%"                                         thawte\pfca2020.crt         roots.sst
%l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%"                                         thawte\sca1998.crt          roots.sst
%l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%"                                         thawte\sca2020.crt          roots.sst
%l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%"                                         thawte\spca1998.crt         roots.sst 
%l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%"                                         thawte\spca2020.crt         roots.sst 

rem --------------------------------------------------------------------------------------------------------------
rem         ***     KEYWITNESS     ***
rem --------------------------------------------------------------------------------------------------------------

%l_CMGR% -eku "%l_SAUTH%,%l_CAUTH%,%l_EMAIL%"                               other\kwitness.crt          roots.sst

rem --------------------------------------------------------------------------------------------------------------
rem         ***     MCI     ***
rem --------------------------------------------------------------------------------------------------------------

%l_CMGR% -eku "%l_SAUTH%,%l_CAUTH%,%l_EMAIL%"                               other\mcimall.crt           roots.sst





echo .
echo . checking in *.sst
echo .

in -c"auto create" *.sst

certmgr -v roots.sst    > roots.txt
certmgr -v cas.sst      > cas.txt
