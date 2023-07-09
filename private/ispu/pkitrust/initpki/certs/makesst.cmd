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

set l_ROOTSTOREFILE=roots.sst
set l_CASTOREFILE=cas.sst

rem echo .
rem echo . checking out *.sst
rem echo .

rem %out *.sst

if exist %l_ROOTSTOREFILE%    del %l_ROOTSTOREFILE%
if exist %l_CASTOREFILE%      del %l_CASTOREFILE%



rem --------------------------------------------------------------------------------------------------------------
rem         ***     VERISIGN     ***
rem --------------------------------------------------------------------------------------------------------------

rem VeriSign certs to flush...

rem This is hash 0x4b281266, old RSA Secure Server CA, expires 12/31/99
rem set l_NAME=VeriSign/RSA Secure Server CA
rem %l_CMGR% -eku "%l_SAUTH%" -name "%l_NAME%"                                  rsa\rsa-ssca.crt            %l_ROOTSTOREFILE%

rem This is hash 0x0884a5f8, old Class 1 Public PCA, expires 12/31/99
rem set l_NAME=VeriSign Class 1 Primary CA
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\class1-v0.509      %l_ROOTSTOREFILE%

rem This is hash 0x127046ed, old Class 1 Public PCA, expires 1/7/2004
rem set l_NAME=VeriSign Class 1 Primary CA
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\class1-v1.509      %l_ROOTSTOREFILE%

rem This is hash , old Class 4 Public PCA, expires 12/31/1999
rem set l_NAME=VeriSign Class 4 Primary CA
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\class4-v1.509      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Commercial Software Publishers CA
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\mscom2004.509      %l_ROOTSTOREFILE%

rem This is hash 0x0fae155f, old Commercial softpub cert, expires 12/31/99
rem We have to continue shipping this root because certs issued off
rem of it use AKI: Issuer & serial number

set l_NAME=VeriSign Commercial Software Publishers CA
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\mscom1999.509      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Individual Software Publishers CA
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\msind2004.509      %l_ROOTSTOREFILE%

rem This is hash 0x438d4e9c, old Individual softpub cert, expires 12/31/99
rem We have to continue shipping this root because certs issued off
rem of it use AKI: Issuer & serial number

set l_NAME=VeriSign Individual Software Publishers CA
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        verisign\msind1999.509      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 1 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\class1-v2.509      %l_ROOTSTOREFILE%

rem This is the VS Class 2 PCA; class2-v1 and class2-v2 are duplicates, 
rem only need one of them.  Hash 0xbbfab727
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\class2-v1.509      %l_ROOTSTOREFILE%
set l_NAME=VeriSign Class 2 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\class2-v2.509      %l_ROOTSTOREFILE%

rem This is the VS Class 3 PCA; class3-v1 and class3-v2 are duplicates, 
rem only need one of them.  Hash 0x4d5f2ab4
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\class3-v1.509      %l_ROOTSTOREFILE%
set l_NAME=VeriSign Class 3 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\class3-v2.509      %l_ROOTSTOREFILE%

rem set l_NAME=VeriSign/RSA Commercial CA
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              rsa\rsa-cca.crt             %l_ROOTSTOREFILE%

set l_NAME=VeriSign/RSA Secure Server CA
%l_CMGR% -eku "%l_SAUTH%" -name "%l_NAME%"                                  rsa\sscav2.509              %l_ROOTSTOREFILE%

rem New certs as of 5/20/98

set l_NAME=VeriSign Class 1 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\c1pca_g2.cer      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 2 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\c2pca_g2.cer      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 3 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\c3pca_g2.cer      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 4 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\c4pca_g2.cer      %l_ROOTSTOREFILE%

rem ------      this is the "us" cert -- we don't want to ship this!
rem ------ set l_NAME=VeriSign Online Revocation Status Service
rem ------ %l_CMGR% -name "%l_NAME%"                                        verisign\crlsign-v1.509     %l_ROOTSTOREFILE%

set l_NAME=VeriSign Time Stamping CA
%l_CMGR% -eku "%l_TSTMP%" -name "%l_NAME%"                                  verisign\timeroot.509       %l_ROOTSTOREFILE%

rem This is the VS Class 1 Intermediate
rem %l_CMGR%                                                                    verisign\class1iv1.509      %l_CASTOREFILE%

rem This is the VS Class 2 Intermediate
rem %l_CMGR%                                                                    verisign\class2iv1.509      %l_CASTOREFILE%

rem Replacing the VS Class 1 intermediate with one expiring on 2008
%l_CMGR%                                                                    verisign\c1i_2008.cer      %l_CASTOREFILE%

rem This is the VS Class 2 Intermediate
rem Replacing the VS Class 2 intermediate with one expiring on 2004
%l_CMGR%                                                                    verisign\c2i_2004.cer      %l_CASTOREFILE%

rem New VS certs as of 7/7/99

set l_NAME=VeriSign Class 1 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\C1PCAG2v2.cer      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 2 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\C2PCAG2v2.cer      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 3 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\C3PCAG2v2.cer	%l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 4 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\C4PCAG2v2.cer	%l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 1 Public Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%" -name "%l_NAME%"                        verisign\PCA1_v4.cer	%l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 2 Public Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%" -name "%l_NAME%"              verisign\PCA2_v4.cer      %l_ROOTSTOREFILE%

set l_NAME=VeriSign Class 3 Public Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_CSIGN%,%l_SAUTH%" -name "%l_NAME%"    verisign\PCA3_v4.cer      %l_ROOTSTOREFILE%

rem VS SGC x-cert to fix SP6 SGC problem
%l_CMGR%                                                                    verisign\c3i_2004.cer      %l_CASTOREFILE%



rem --------------------------------------------------------------------------------------------------------------
rem         ***     MICROSOFT     ***
rem --------------------------------------------------------------------------------------------------------------

set l_NAME=Microsoft Authenticode(tm) Root
%l_CMGR% -eku "%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"                        msft\msroot99.cer           %l_ROOTSTOREFILE%

set l_NAME=Microsoft Timestamp Root
%l_CMGR% -eku "%l_TSTMP%" -name "%l_NAME%"                                  msft\hawking.cer            %l_ROOTSTOREFILE%

rem SGC root removed (Win2k 387794 & WinSE 3715)
rem This is the MS Root for Server-Gated Crypto (SGC)
rem set l_NAME=Microsoft Root SGC Authority
rem %l_CMGR% -eku "%l_SAUTH%,%l_SVRGT%,%l_NETSC%" -name "%l_NAME%"              msft\sgcroot.crt            %l_ROOTSTOREFILE%

rem This is the SGC intermediate certificate
%l_CMGR% -eku "%l_SAUTH%,%l_SVRGT%,%l_NETSC%"                               msft\sgc_ca.crt             %l_CASTOREFILE%

rem This is the MS Root Authority (calling it the WHQL root is a misnomer).  
rem It expires in 2020
set l_NAME=Microsoft Root Authority
%l_CMGR% -name "%l_NAME%"                                                   msft\whqlroot.cer           %l_ROOTSTOREFILE%

rem This is the WHQL intermediate cert (chains off the MS Root Authority),
rem used for things like Memphis driver signing, MS publishing, etc.
set l_NAME=Microsoft Windows Hardware Compatibility
%l_CMGR%                                                                    msft\whqlint.cer            %l_CASTOREFILE%

rem %l_CMGR%                                                                msft\mstemp.cer             %l_CASTOREFILE%

%l_CMGR%                                                                    test\mstest.cer             %l_CASTOREFILE%


rem --------------------------------------------------------------------------------------------------------------
rem         ***     GTE     ***
rem --------------------------------------------------------------------------------------------------------------

rem this is an old GTE root, hash 0x129c55b6, expires 12/31/99
rem we're keeping it this go-round while GTE migrates to a new key.

rem set l_NAME=GTE CyberTrust Root
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              gte\ct_root.cer             %l_ROOTSTOREFILE%

rem this is the new GTE root, hash, expires 4/4/2004
set l_NAME=GTE CyberTrust Root
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              gte\ct200404.cer             %l_ROOTSTOREFILE%

set l_NAME=GTE CyberTrust Root
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              gte\ct200602.cer             %l_ROOTSTOREFILE%

set l_NAME=GTE CyberTrust Global Root
%l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              gte\ct201808.cer             %l_ROOTSTOREFILE%

rem GTE SGC bridge cert
%l_CMGR%								    gte\gtebridge.cer		%l_CASTOREFILE%


rem --------------------------------------------------------------------------------------------------------------
rem         ***     ATT     ***
rem --------------------------------------------------------------------------------------------------------------

rem These certificates (0x7c76ed02 and 0x8dd3f0c5) expire in 1/16/01 and 12/31/99; we're not carrying any AT&T root certs any more (at least, they haven't replaced any for NT5 B2)
rem set l_NAME=ATT Certificate Services
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              att\att.crt                 %l_ROOTSTOREFILE%
rem set l_NAME=ATT Directory Services
rem %l_CMGR% -eku "%l_EMAIL%,%l_CAUTH%,%l_SAUTH%" -name "%l_NAME%"              att\attdir.crt              %l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     THAWTE     ***
rem --------------------------------------------------------------------------------------------------------------

rem this is an old Thawte premium server root, hash 0xd1dc53dc, expires 7/27/98
rem %l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%"                                         thawte\spca1998.crt         %l_ROOTSTOREFILE% 
rem this is an old Thawte server CA root, hash 0x9008b1f0, expires 7/27/98
rem %l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%"                                         thawte\sca1998.crt          %l_ROOTSTOREFILE%

set l_NAME=Thawte Personal Basic CA
%l_CMGR% -eku "%l_CAUTH%,%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"              thawte\pbca2020.crt         %l_ROOTSTOREFILE%
set l_NAME=Thawte Personal Premium CA
%l_CMGR% -eku "%l_CAUTH%,%l_EMAIL%,%l_CSIGN%" -name "%l_NAME%"              thawte\ppca2020.crt         %l_ROOTSTOREFILE%
set l_NAME=Thawte Personal Freemail CA
%l_CMGR% -eku "%l_CAUTH%,%l_EMAIL%" -name "%l_NAME%"                        thawte\pfca2020.crt         %l_ROOTSTOREFILE%
set l_NAME=Thawte Server CA
%l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%" -name "%l_NAME%"                        thawte\sca2020.crt          %l_ROOTSTOREFILE%
set l_NAME=Thawte Premium Server CA
%l_CMGR% -eku "%l_SAUTH%,%l_CSIGN%" -name "%l_NAME%"                        thawte\spca2020.crt         %l_ROOTSTOREFILE% 
set l_NAME=Thawte Timestamping CA
%l_CMGR% -eku "%l_TSTMP%" -name "%l_NAME%"				    thawte\ts2020.cer           %l_ROOTSTOREFILE% 

rem Thawte SGC bridge cert
%l_CMGR%								    thawte\sgc1.cer		%l_CASTOREFILE%

rem Thawte SGC premium bridge cert
%l_CMGR%								    thawte\prem_sgc.cer		%l_CASTOREFILE%


rem --------------------------------------------------------------------------------------------------------------
rem         ***     KEYWITNESS     ***
rem --------------------------------------------------------------------------------------------------------------

rem Keywitness is out of business. Do not add this CA back into the product in the future.

rem *** KeyWitness removed on 7/7/99 ***

rem this is the old KeyWitness root, hash 0xBDCD5DEA, expires 5/6/99
rem %l_CMGR% -eku "%l_SAUTH%,%l_CAUTH%,%l_EMAIL%"                               other\kwitness.crt          %l_ROOTSTOREFILE%

rem this is the new KeyWitness root, hash 0x06d81263, expires 5/5/2004
rem set l_NAME=KeyWitness Global 2048 Root
rem %l_CMGR% -eku "%l_SAUTH%,%l_CAUTH%,%l_EMAIL%" -name "%l_NAME%"                               other\kw2004.cer          %l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     MCI     ***
rem --------------------------------------------------------------------------------------------------------------

rem This is an old MCI root cert, hash 0x6357d33d, expires 7/16/98
rem %l_CMGR% -eku "%l_SAUTH%,%l_CAUTH%,%l_EMAIL%"                               other\mcimall.crt               %l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     CertiPoste      ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99 and will expire on 6/24/2018
set l_NAME=Certiposte Classe A Personne
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	certip\certip1.cer		%l_ROOTSTOREFILE%

set l_NAME=Certiposte Serveur
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	certip\certip1.cer		%l_ROOTSTOREFILE%

set l_NAME=Certiposte Editeur
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	certip\certip1.cer		%l_ROOTSTOREFILE%

set l_NAME=Certiposte Serveur
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	certip\sagemroot.crt		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Correos      ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99 and will expire on 6/24/2018
set l_NAME=SERVICIOS DE CERTIFICACION - A.N.C.
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Correos\ca.crt			%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Digital Signature Trust     ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=DST (ANX Network) CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\ANX.cer			%l_ROOTSTOREFILE%

set l_NAME=DSTCA E1
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\DSTCAE1.cer		%l_ROOTSTOREFILE%

set l_NAME=DSTCA E2
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\DSTCAE2.cer		%l_ROOTSTOREFILE%

set l_NAME=DST-Entrust GTI CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\DSTEntrst.cer		%l_ROOTSTOREFILE%

set l_NAME=DST RootCA X1
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\DSTXCA1.cer		%l_ROOTSTOREFILE%

set l_NAME=DST RootCA X2
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\DSTXCA2.cer		%l_ROOTSTOREFILE%

set l_NAME=Xcert EZ by DST
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\X3CER.cer		%l_ROOTSTOREFILE%

set l_NAME=DST (National Retail Federation) RootCA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\NRF.cer			%l_ROOTSTOREFILE%

set l_NAME=DST (United Parcel Service) RootCA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\UPS.cer			%l_ROOTSTOREFILE%

rem these certs are added 7/12/99
set l_NAME=DST (ABA.ECOM) CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\ABACER.cer		%l_ROOTSTOREFILE%

set l_NAME=DST (Baltimore EZ) CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DigSigT\baltimore.cer		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Equifax     ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=Equifax Secure eBusiness CA-1
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Equifax\ebus_ca1.cer		%l_ROOTSTOREFILE%

set l_NAME=Equifax Secure eBusiness CA-2
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Equifax\ebus_ca2.cer		%l_ROOTSTOREFILE%

set l_NAME=Equifax Secure Global eBusiness CA-1
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Equifax\gebus_ca1.cer		%l_ROOTSTOREFILE%

set l_NAME=Equifax Secure Certificate Authority
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Equifax\sec_ca.cer		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     First Data Digital Certificates Inc.    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=First Data Digital Certificates Inc. Certification Authority
%l_CMGR% -name "%l_NAME%"			FDC\ca.cer			%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     FNMT     ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=Fabrica Nacional de Moneda y Timbre
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	FNMT\fnmt.cer			%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     GlobalSign    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=GlobalSign Root CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	GlobalS\root.cer			%l_ROOTSTOREFILE%

rem GlobalSign SGC bridge cert
%l_CMGR%								GlobalS\gbridge.cer		%l_CASTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Japan Certification Services    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=Japan Certification Services, Inc. SecureSign RootCA1
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"			JCS\jcsca1.der		%l_ROOTSTOREFILE%

set l_NAME=Japan Certification Services, Inc. SecureSign RootCA2
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"			JCS\jcsca2.der		%l_ROOTSTOREFILE%

set l_NAME=Japan Certification Services, Inc. SecureSign RootCA3
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"			JCS\jcsca3.der		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     KeyMail    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=KeyMail PTT Post Root CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	KeyMail\PTTCA.CRT	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     National Association of Mexican Notary    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=Autoridad Certificadora de la Asociacion Nacional del Notariado 
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	NAMx\ANNM.cer		%l_ROOTSTOREFILE%

set l_NAME=Autoridad Certificadora del Colegio Nacional de Correduria Publica Mexicana, A.C.
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	NAMx\CNCPM.cer		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Saunalahden Serveri    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=Saunalahden Serveri CA 
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SFS\goldnew.cer		%l_ROOTSTOREFILE%

set l_NAME=Saunalahden Serveri CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SFS\silvernew.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Societa Interbancaria per l'Automazione     ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=Societa Interbancaria per l'Automazione SIA Secure Client CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SIA\seccli.der		%l_ROOTSTOREFILE%

rem this cert is added 7/12/99
set l_NAME=Societa Interbancaria per l'Automazione SIA Secure Server CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SIA\secsrv.der		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Valicert    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/7/99
set l_NAME=ValiCert Class 1 Policy Validation Authority
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	ValiCert\class1.cer	%l_ROOTSTOREFILE%

set l_NAME=ValiCert Class 2 Policy Validation Authority
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	ValiCert\class2.cer	%l_ROOTSTOREFILE%

set l_NAME=ValiCert Class 3 Policy Validation Authority
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	ValiCert\class3.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Belgacom    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=Belgacom E-Trust Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Belgacom\primary.crt	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     CertiSign    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=Certisign Autoridade Certificadora AC1S
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertiSign\AC1S.der	%l_ROOTSTOREFILE%

set l_NAME=Certisign Autoridade Certificadora AC2
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertiSign\AC2.der	%l_ROOTSTOREFILE%

set l_NAME=Certisign Autoridade Certificadora AC3S
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertiSign\AC3S.der	%l_ROOTSTOREFILE%

set l_NAME=Certisign Autoridade Certificadora AC4
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertiSign\AC4.der	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     CertPlus    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=CertPlus Class 1 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertPlus\class1.cer	%l_ROOTSTOREFILE%

set l_NAME=CertPlus Class 2 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertPlus\class2.cer	%l_ROOTSTOREFILE%

set l_NAME=CertPlus Class 3 Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertPlus\class3.cer	%l_ROOTSTOREFILE%

set l_NAME=CertPlus Class 3P Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertPlus\class3p.cer	%l_ROOTSTOREFILE%

set l_NAME=CertPlus Class 3TS Primary CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	CertPlus\class3ts.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Deutsche Telekom    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=Deutsche Telekom Root CA 1
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DeutscheT\DTroot1.cer	%l_ROOTSTOREFILE%

set l_NAME=Deutsche Telekom Root CA 2
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	DeutscheT\DTroot2.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Entrust.net    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=Entrust.net Secure Server Certification Authority
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Entrust\entrust.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     EUnet    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=EUnet International Root CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	EUNet\rootEUI.crt	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Feste    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=FESTE, Verified Certs
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Feste\cacert1.der	%l_ROOTSTOREFILE%

set l_NAME=FESTE, Public Notary Certs
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Feste\cacert2.der	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     IPS    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=IPS SERVIDORES
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	IPS\root.cer		%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     SecureNet (Australia)   ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=SecureNet CA Class A
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNet\classAv1.cer	%l_ROOTSTOREFILE%

set l_NAME=SecureNet CA Class B
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNet\classBv1.cer	%l_ROOTSTOREFILE%

set l_NAME=SecureNet CA Root
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNet\Rootv3.cer	%l_ROOTSTOREFILE%

set l_NAME=SecureNet CA SGC Root
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNet\RootSGC.cer	%l_ROOTSTOREFILE%

rem Rotek SGC bridge cert
%l_CMGR%								SecNet\rbridge.cer		%l_CASTOREFILE%


rem --------------------------------------------------------------------------------------------------------------
rem         ***     SecureNet (Hong Kong)   ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=CW HKT SecureNet CA Class A
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNetCW\classA.cer	%l_ROOTSTOREFILE%

set l_NAME=CW HKT SecureNet CA Class B
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNetCW\classB.cer	%l_ROOTSTOREFILE%

set l_NAME=CW HKT SecureNet CA Root
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNetCW\Root.cer	%l_ROOTSTOREFILE%

set l_NAME=CW HKT SecureNet CA SGC Root
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SecNetCW\RootSGC.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     SwissKey   ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=Swisskey Root CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	SwissKey\root.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     TC TrustCenter   ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=TC TrustCenter Class 1 CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	TCTrust\tc_lot_1.cer	%l_ROOTSTOREFILE%

set l_NAME=TC TrustCenter Class 2 CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	TCTrust\tc_lot_2.cer	%l_ROOTSTOREFILE%

set l_NAME=TC TrustCenter Class 3 CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	TCTrust\tc_lot_3.cer	%l_ROOTSTOREFILE%

set l_NAME=TC TrustCenter Class 4 CA
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	TCTrust\tc_lot_4.cer	%l_ROOTSTOREFILE%

set l_NAME=TC TrustCenter Time Stamping CA
%l_CMGR% -eku "%l_TSTMP%" -name "%l_NAME%"				TCTrust\tc_lot_ts.cer	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     Viacode   ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=ViaCode Certification Authority
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	Viacode\root.crt	%l_ROOTSTOREFILE%

rem --------------------------------------------------------------------------------------------------------------
rem         ***     UserTrust     ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/12/99
set l_NAME=UTN - DATACorp SGC
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	UserTrust\rootsgc.cer	%l_ROOTSTOREFILE%

set l_NAME=UTN - USERFirst-Client Authentication and Email
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	UserTrust\cli_e.cer	%l_ROOTSTOREFILE%

set l_NAME=UTN - USERFirst-Hardware
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	UserTrust\hardware.cer	%l_ROOTSTOREFILE%

set l_NAME=UTN - USERFirst-Network Applications
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	UserTrust\netapp.cer	%l_ROOTSTOREFILE%

set l_NAME=UTN - USERFirst-Object
%l_CMGR% -eku "%l_CSIGN%,%l_TSTMP%" -name "%l_NAME%"			UserTrust\object.cer	%l_ROOTSTOREFILE%

rem UserTrust SGC bridge cert
%l_CMGR%								UserTrust\utbridge.cer		%l_CASTOREFILE%


rem --------------------------------------------------------------------------------------------------------------
rem         ***     NetLock    ***
rem --------------------------------------------------------------------------------------------------------------

rem these certs are added 7/15/99
set l_NAME=NetLock Kozjegyzoi (Class A) Tanusitvanykiado
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	NetLock\classa.cer	%l_ROOTSTOREFILE%

set l_NAME=NetLock Uzleti (Class B) Tanusitvanykiado
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	NetLock\classb.cer	%l_ROOTSTOREFILE%

set l_NAME=NetLock Expressz (Class C) Tanusitvanykiado
%l_CMGR% -eku "%l_EMAIL%,%l_SAUTH%" -name "%l_NAME%"              	NetLock\classc.cer	%l_ROOTSTOREFILE%


rem --------------------------------------------------------------------------------------------------------------
rem         ***     .sst file checkin      ***
rem --------------------------------------------------------------------------------------------------------------

rem echo .
rem echo . checking in *.sst
rem echo .

rem %in -c"auto create" *.sst

certmgr -v %l_ROOTSTOREFILE%    > roots.txt
certmgr -v %l_CASTOREFILE%      > cas.txt
