/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>
#ifdef __REACTOS__
#include <wchar.h>
#endif

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winsock2.h"
#include "ws2tcpip.h"
#include "initguid.h"
#include "wbemcli.h"
#include "wbemprov.h"
#include "iphlpapi.h"
#include "netioapi.h"
#include "tlhelp32.h"
#ifndef __REACTOS__
#include "d3d10.h"
#endif
#include "winternl.h"
#include "winioctl.h"
#include "winsvc.h"
#include "winver.h"
#include "sddl.h"
#include "ntsecapi.h"
#ifdef __REACTOS__
#include <wingdi.h>
#include <winreg.h>
#endif
#include "winspool.h"
#include "setupapi.h"

#include "wine/asm.h"
#include "wine/debug.h"
#include "wbemprox_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wbemprox);

static const WCHAR class_associatorsW[] =
    {'_','_','A','S','S','O','C','I','A','T','O','R','S',0};
static const WCHAR class_baseboardW[] =
    {'W','i','n','3','2','_','B','a','s','e','B','o','a','r','d',0};
static const WCHAR class_biosW[] =
    {'W','i','n','3','2','_','B','I','O','S',0};
static const WCHAR class_cdromdriveW[] =
    {'W','i','n','3','2','_','C','D','R','O','M','D','r','i','v','e',0};
static const WCHAR class_compsysW[] =
    {'W','i','n','3','2','_','C','o','m','p','u','t','e','r','S','y','s','t','e','m',0};
static const WCHAR class_compsysproductW[] =
    {'W','i','n','3','2','_','C','o','m','p','u','t','e','r','S','y','s','t','e','m','P','r','o','d','u','c','t',0};
static const WCHAR class_datafileW[] =
    {'C','I','M','_','D','a','t','a','F','i','l','e',0};
static const WCHAR class_desktopmonitorW[] =
    {'W','i','n','3','2','_','D','e','s','k','t','o','p','M','o','n','i','t','o','r',0};
static const WCHAR class_directoryW[] =
    {'W','i','n','3','2','_','D','i','r','e','c','t','o','r','y',0};
static const WCHAR class_diskdriveW[] =
    {'W','i','n','3','2','_','D','i','s','k','D','r','i','v','e',0};
static const WCHAR class_diskdrivetodiskpartitionW[] =
    {'W','i','n','3','2','_','D','i','s','k','D','r','i','v','e','T','o','D','i','s','k','P','a','r','t','i','t','i','o','n',0};
static const WCHAR class_diskpartitionW[] =
    {'W','i','n','3','2','_','D','i','s','k','P','a','r','t','i','t','i','o','n',0};
static const WCHAR class_displaycontrollerconfigW[] =
    {'W','i','n','3','2','_','D','i','s','p','l','a','y','C','o','n','t','r','o','l','l','e','r',
     'C','o','n','f','i','g','u','r','a','t','i','o','n',0};
static const WCHAR class_ip4routetableW[] =
    {'W','i','n','3','2','_','I','P','4','R','o','u','t','e','T','a','b','l','e',0};
static const WCHAR class_logicaldiskW[] =
    {'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k',0};
static const WCHAR class_logicaldisk2W[] =
    {'C','I','M','_','L','o','g','i','c','a','l','D','i','s','k',0};
static const WCHAR class_logicaldisktopartitionW[] =
    {'W','i','n','3','2','_','L','o','g','i','c','a','l','D','i','s','k','T','o','P','a','r','t','i','t','i','o','n',0};
static const WCHAR class_networkadapterW[] =
    {'W','i','n','3','2','_','N','e','t','w','o','r','k','A','d','a','p','t','e','r',0};
static const WCHAR class_networkadapterconfigW[] =
    {'W','i','n','3','2','_','N','e','t','w','o','r','k','A','d','a','p','t','e','r',
     'C','o','n','f','i','g','u','r','a','t','i','o','n',0};
static const WCHAR class_operatingsystemW[] =
    {'W','i','n','3','2','_','O','p','e','r','a','t','i','n','g','S','y','s','t','e','m',0};
static const WCHAR class_paramsW[] =
    {'_','_','P','A','R','A','M','E','T','E','R','S',0};
static const WCHAR class_physicalmediaW[] =
    {'W','i','n','3','2','_','P','h','y','s','i','c','a','l','M','e','d','i','a',0};
static const WCHAR class_physicalmemoryW[] =
    {'W','i','n','3','2','_','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
static const WCHAR class_pnpentityW[] =
    {'W','i','n','3','2','_','P','n','P','E','n','t','i','t','y',0};
static const WCHAR class_printerW[] =
    {'W','i','n','3','2','_','P','r','i','n','t','e','r',0};
static const WCHAR class_process_getowner_outW[] =
    {'_','_','W','I','N','3','2','_','P','R','O','C','E','S','S','_','G','E','T','O','W',
     'N','E','R','_','O','U','T',0};
static const WCHAR class_processorW[] =
    {'W','i','n','3','2','_','P','r','o','c','e','s','s','o','r',0};
static const WCHAR class_processor2W[] =
    {'C','I','M','_','P','r','o','c','e','s','s','o','r',0};
static const WCHAR class_qualifiersW[] =
    {'_','_','Q','U','A','L','I','F','I','E','R','S',0};
static const WCHAR class_quickfixengineeringW[] =
    {'W','i','n','3','2','_','Q','u','i','c','k','F','i','x','E','n','g','i','n','e','e','r','i','n','g',0};
static const WCHAR class_sidW[] =
    {'W','i','n','3','2','_','S','I','D',0};
static const WCHAR class_sounddeviceW[] =
    {'W','i','n','3','2','_','S','o','u','n','d','D','e','v','i','c','e',0};
static const WCHAR class_systemenclosureW[] =
    {'W','i','n','3','2','_','S','y','s','t','e','m','E','n','c','l','o','s','u','r','e',0};
#ifndef __REACTOS__
static const WCHAR class_videocontrollerW[] =
    {'W','i','n','3','2','_','V','i','d','e','o','C','o','n','t','r','o','l','l','e','r',0};
#endif
static const WCHAR class_winsatW[] =
    {'W','i','n','3','2','_','W','i','n','S','A','T',0};

static const WCHAR prop_accountnameW[] =
    {'A','c','c','o','u','n','t','N','a','m','e',0};
static const WCHAR prop_acceptpauseW[] =
    {'A','c','c','e','p','t','P','a','u','s','e',0};
static const WCHAR prop_acceptstopW[] =
    {'A','c','c','e','p','t','S','t','o','p',0};
static const WCHAR prop_accessmaskW[] =
    {'A','c','c','e','s','s','M','a','s','k',0};
#ifndef __REACTOS__
static const WCHAR prop_adapterdactypeW[] =
    {'A','d','a','p','t','e','r','D','A','C','T','y','p','e',0};
static const WCHAR prop_adapterramW[] =
    {'A','d','a','p','t','e','r','R','A','M',0};
#endif
static const WCHAR prop_adaptertypeW[] =
    {'A','d','a','p','t','e','r','T','y','p','e',0};
static const WCHAR prop_adaptertypeidW[] =
    {'A','d','a','p','t','e','r','T','y','p','e','I','D',0};
static const WCHAR prop_addresswidthW[] =
    {'A','d','d','r','e','s','s','W','i','d','t','h',0};
static const WCHAR prop_antecedentW[] =
    {'A','n','t','e','c','e','d','e','n','t',0};
static const WCHAR prop_architectureW[] =
    {'A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR prop_assocclassW[] =
    {'A','s','s','o','c','C','l','a','s','s',0};
static const WCHAR prop_associatorW[] =
    {'A','s','s','o','c','i','a','t','o','r',0};
static const WCHAR prop_attributesW[] =
    {'A','t','t','r','i','b','u','t','e','s',0};
#ifndef __REACTOS__
static const WCHAR prop_availabilityW[] =
    {'A','v','a','i','l','a','b','i','l','i','t','y',0};
#endif
static const WCHAR prop_binaryrepresentationW[] =
    {'B','i','n','a','r','y','R','e','p','r','e','s','e','n','t','a','t','i','o','n',0};
static const WCHAR prop_bitsperpixelW[] =
    {'B','i','t','s','P','e','r','P','i','x','e','l',0};
static const WCHAR prop_boolvalueW[] =
    {'B','o','o','l','V','a','l','u','e',0};
static const WCHAR prop_bootableW[] =
    {'B','o','o','t','a','b','l','e',0};
static const WCHAR prop_bootpartitionW[] =
    {'B','o','o','t','P','a','r','t','i','t','i','o','n',0};
static const WCHAR prop_buildnumberW[] =
    {'B','u','i','l','d','N','u','m','b','e','r',0};
static const WCHAR prop_capacityW[] =
    {'C','a','p','a','c','i','t','y',0};
static const WCHAR prop_captionW[] =
    {'C','a','p','t','i','o','n',0};
static const WCHAR prop_chassistypesW[] =
    {'C','h','a','s','s','i','s','T','y','p','e','s',0};
static const WCHAR prop_classW[] =
    {'C','l','a','s','s',0};
static const WCHAR prop_codesetW[] =
    {'C','o','d','e','S','e','t',0};
static const WCHAR prop_commandlineW[] =
    {'C','o','m','m','a','n','d','L','i','n','e',0};
static const WCHAR prop_configmanagererrorcodeW[] =
    {'C','o','n','f','i','g','M','a','n','a','g','e','r','E','r','r','o','r','C','o','d','e',0};
static const WCHAR prop_configuredclockspeedW[] =
    {'C','o','n','f','i','g','u','r','e','d','C','l','o','c','k','S','p','e','e','d',0};
static const WCHAR prop_countrycodeW[] =
    {'C','o','u','n','t','r','y','C','o','d','e',0};
static const WCHAR prop_cpuscoreW[] =
    {'C','P','U','S','c','o','r','e',0};
static const WCHAR prop_cpustatusW[] =
    {'C','p','u','S','t','a','t','u','s',0};
static const WCHAR prop_csdversionW[] =
    {'C','S','D','V','e','r','s','i','o','n',0};
static const WCHAR prop_csnameW[] =
    {'C','S','N','a','m','e',0};
#ifndef __REACTOS__
static const WCHAR prop_currentbitsperpixelW[] =
    {'C','u','r','r','e','n','t','B','i','t','s','P','e','r','P','i','x','e','l',0};
#endif
static const WCHAR prop_currentclockspeedW[] =
    {'C','u','r','r','e','n','t','C','l','o','c','k','S','p','e','e','d',0};
static const WCHAR prop_currenthorizontalresW[] =
    {'C','u','r','r','e','n','t','H','o','r','i','z','o','n','t','a','l','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR prop_currentlanguageW[] =
    {'C','u','r','r','e','n','t','L','a','n','g','u','a','g','e',0};
static const WCHAR prop_currentrefreshrateW[] =
    {'C','u','r','r','e','n','t','R','e','f','r','e','s','h','R','a','t','e',0};
static const WCHAR prop_currentscanmodeW[] =
    {'C','u','r','r','e','n','t','S','c','a','n','M','o','d','e',0};
static const WCHAR prop_currenttimezoneW[] =
    {'C','u','r','r','e','n','t','T','i','m','e','Z','o','n','e',0};
static const WCHAR prop_currentverticalresW[] =
    {'C','u','r','r','e','n','t','V','e','r','t','i','c','a','l','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR prop_d3dscoreW[] =
    {'D','3','D','S','c','o','r','e',0};
static const WCHAR prop_datawidthW[] =
    {'D','a','t','a','W','i','d','t','h',0};
static const WCHAR prop_defaultipgatewayW[] =
    {'D','e','f','a','u','l','t','I','P','G','a','t','e','w','a','y',0};
static const WCHAR prop_defaultvalueW[] =
    {'D','e','f','a','u','l','t','V','a','l','u','e',0};
static const WCHAR prop_dependentW[] =
    {'D','e','p','e','n','d','e','n','t',0};
static const WCHAR prop_descriptionW[] =
    {'D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR prop_destinationW[] =
    {'D','e','s','t','i','n','a','t','i','o','n',0};
static const WCHAR prop_deviceidW[] =
    {'D','e','v','i','c','e','I','d',0};
static const WCHAR prop_devicelocatorW[] =
    {'D','e','v','i','c','e','L','o','c','a','t','o','r',0};
static const WCHAR prop_dhcpenabledW[] =
    {'D','H','C','P','E','n','a','b','l','e','d',0};
static const WCHAR prop_directionW[] =
    {'D','i','r','e','c','t','i','o','n',0};
static const WCHAR prop_diskscoreW[] =
    {'D','i','s','k','S','c','o','r','e',0};
static const WCHAR prop_displaynameW[] =
    {'D','i','s','p','l','a','y','N','a','m','e',0};
static const WCHAR prop_diskindexW[] =
    {'D','i','s','k','I','n','d','e','x',0};
static const WCHAR prop_dnshostnameW[] =
    {'D','N','S','H','o','s','t','N','a','m','e',0};
static const WCHAR prop_dnsserversearchorderW[] =
    {'D','N','S','S','e','r','v','e','r','S','e','a','r','c','h','O','r','d','e','r',0};
static const WCHAR prop_domainW[] =
    {'D','o','m','a','i','n',0};
static const WCHAR prop_domainroleW[] =
    {'D','o','m','a','i','n','R','o','l','e',0};
static const WCHAR prop_driveW[] =
    {'D','r','i','v','e',0};
static const WCHAR prop_driverdateW[] =
    {'D','r','i','v','e','r','D','a','t','e',0};
static const WCHAR prop_drivernameW[] =
    {'D','r','i','v','e','r','N','a','m','e',0};
#ifndef __REACTOS__
static const WCHAR prop_driverversionW[] =
    {'D','r','i','v','e','r','V','e','r','s','i','o','n',0};
#endif
static const WCHAR prop_drivetypeW[] =
    {'D','r','i','v','e','T','y','p','e',0};
static const WCHAR prop_familyW[] =
    {'F','a','m','i','l','y',0};
static const WCHAR prop_filesystemW[] =
    {'F','i','l','e','S','y','s','t','e','m',0};
static const WCHAR prop_flavorW[] =
    {'F','l','a','v','o','r',0};
static const WCHAR prop_freespaceW[] =
    {'F','r','e','e','S','p','a','c','e',0};
static const WCHAR prop_freephysicalmemoryW[] =
    {'F','r','e','e','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
static const WCHAR prop_handleW[] =
    {'H','a','n','d','l','e',0};
static const WCHAR prop_graphicsscoreW[] =
    {'G','r','a','p','h','i','c','s','S','c','o','r','e',0};
static const WCHAR prop_horizontalresolutionW[] =
    {'H','o','r','i','z','o','n','t','a','l','R','e','s','o','l','u','t','i','o','n',0};
static const WCHAR prop_hotfixidW[] =
    {'H','o','t','F','i','x','I','D',0};
static const WCHAR prop_idW[] =
    {'I','D',0};
static const WCHAR prop_identificationcodeW[] =
    {'I','d','e','n','t','i','f','i','c','a','t','i','o','n','C','o','d','e',0};
static const WCHAR prop_identifyingnumberW[] =
    {'I','d','e','n','t','i','f','y','i','n','g','N','u','m','b','e','r',0};
static const WCHAR prop_indexW[] =
    {'I','n','d','e','x',0};
static const WCHAR prop_installdateW[] =
    {'I','n','s','t','a','l','l','D','a','t','e',0};
static const WCHAR prop_installeddisplaydriversW[]=
    {'I','n','s','t','a','l','l','e','d','D','i','s','p','l','a','y','D','r','i','v','e','r','s',0};
static const WCHAR prop_interfaceindexW[] =
    {'I','n','t','e','r','f','a','c','e','I','n','d','e','x',0};
static const WCHAR prop_interfacetypeW[] =
    {'I','n','t','e','r','f','a','c','e','T','y','p','e',0};
static const WCHAR prop_intvalueW[] =
    {'I','n','t','e','g','e','r','V','a','l','u','e',0};
static const WCHAR prop_ipaddressW[] =
    {'I','P','A','d','d','r','e','s','s',0};
static const WCHAR prop_ipconnectionmetricW[] =
    {'I','P','C','o','n','n','e','c','t','i','o','n','M','e','t','r','i','c',0};
static const WCHAR prop_ipenabledW[] =
    {'I','P','E','n','a','b','l','e','d',0};
static const WCHAR prop_ipsubnet[] =
    {'I','P','S','u','b','n','e','t',0};
static const WCHAR prop_lastbootuptimeW[] =
    {'L','a','s','t','B','o','o','t','U','p','T','i','m','e',0};
static const WCHAR prop_levelW[] =
    {'L','e','v','e','l',0};
static const WCHAR prop_localW[] =
    {'L','o','c','a','l',0};
static const WCHAR prop_localdatetimeW[] =
    {'L','o','c','a','l','D','a','t','e','T','i','m','e',0};
static const WCHAR prop_localeW[] =
    {'L','o','c','a','l','e',0};
static const WCHAR prop_locationW[] =
    {'L','o','c','a','t','i','o','n',0};
static const WCHAR prop_lockpresentW[] =
    {'L','o','c','k','P','r','e','s','e','n','t',0};
static const WCHAR prop_macaddressW[] =
    {'M','A','C','A','d','d','r','e','s','s',0};
static const WCHAR prop_manufacturerW[] =
    {'M','a','n','u','f','a','c','t','u','r','e','r',0};
static const WCHAR prop_maxclockspeedW[] =
    {'M','a','x','C','l','o','c','k','S','p','e','e','d',0};
static const WCHAR prop_mediatypeW[] =
    {'M','e','d','i','a','T','y','p','e',0};
static const WCHAR prop_memberW[] =
    {'M','e','m','b','e','r',0};
static const WCHAR prop_memoryscoreW[] =
    {'M','e','m','o','r','y','S','c','o','r','e',0};
static const WCHAR prop_memorytypeW[] =
    {'M','e','m','o','r','y','T','y','p','e',0};
static const WCHAR prop_methodW[] =
    {'M','e','t','h','o','d',0};
static const WCHAR prop_modelW[] =
    {'M','o','d','e','l',0};
static const WCHAR prop_netconnectionstatusW[] =
    {'N','e','t','C','o','n','n','e','c','t','i','o','n','S','t','a','t','u','s',0};
static const WCHAR prop_networkW[] =
    {'N','e','t','w','o','r','k',0};
static const WCHAR prop_nexthopW[] =
    {'N','e','x','t','H','o','p',0};
static const WCHAR prop_numcoresW[] =
    {'N','u','m','b','e','r','O','f','C','o','r','e','s',0};
static const WCHAR prop_numlogicalprocessorsW[] =
    {'N','u','m','b','e','r','O','f','L','o','g','i','c','a','l','P','r','o','c','e','s','s','o','r','s',0};
static const WCHAR prop_numprocessorsW[] =
    {'N','u','m','b','e','r','O','f','P','r','o','c','e','s','s','o','r','s',0};
static const WCHAR prop_operatingsystemskuW[] =
    {'O','p','e','r','a','t','i','n','g','S','y','s','t','e','m','S','K','U',0};
static const WCHAR prop_osarchitectureW[] =
    {'O','S','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR prop_oslanguageW[] =
    {'O','S','L','a','n','g','u','a','g','e',0};
static const WCHAR prop_osproductsuiteW[] =
    {'O','S','P','r','o','d','u','c','t','S','u','i','t','e',0};
static const WCHAR prop_ostypeW[] =
    {'O','S','T','y','p','e',0};
static const WCHAR prop_parameterW[] =
    {'P','a','r','a','m','e','t','e','r',0};
static const WCHAR prop_partnumberW[] =
    {'P','a','r','t','N','u','m','b','e','r',0};
static const WCHAR prop_physicaladapterW[] =
    {'P','h','y','s','i','c','a','l','A','d','a','p','t','e','r',0};
static const WCHAR prop_pixelsperxlogicalinchW[] =
    {'P','i','x','e','l','s','P','e','r','X','L','o','g','i','c','a','l','I','n','c','h',0};
static const WCHAR prop_pnpdeviceidW[] =
    {'P','N','P','D','e','v','i','c','e','I','D',0};
static const WCHAR prop_portnameW[] =
    {'P','o','r','t','N','a','m','e',0};
static const WCHAR prop_pprocessidW[] =
    {'P','a','r','e','n','t','P','r','o','c','e','s','s','I','D',0};
static const WCHAR prop_primaryW[] =
    {'P','r','i','m','a','r','y',0};
static const WCHAR prop_processidW[] =
    {'P','r','o','c','e','s','s','I','D',0};
static const WCHAR prop_processoridW[] =
    {'P','r','o','c','e','s','s','o','r','I','d',0};
static const WCHAR prop_processortypeW[] =
    {'P','r','o','c','e','s','s','o','r','T','y','p','e',0};
static const WCHAR prop_productW[] =
    {'P','r','o','d','u','c','t',0};
static const WCHAR prop_productnameW[] =
    {'P','r','o','d','u','c','t','N','a','m','e',0};
static const WCHAR prop_referenceddomainnameW[] =
    {'R','e','f','e','r','e','n','c','e','d','D','o','m','a','i','n','N','a','m','e',0};
static const WCHAR prop_releasedateW[] =
    {'R','e','l','e','a','s','e','D','a','t','e',0};
static const WCHAR prop_revisionW[] =
    {'R','e','v','i','s','i','o','n',0};
static const WCHAR prop_serialnumberW[] =
    {'S','e','r','i','a','l','N','u','m','b','e','r',0};
static const WCHAR prop_servicepackmajorW[] =
    {'S','e','r','v','i','c','e','P','a','c','k','M','a','j','o','r','V','e','r','s','i','o','n',0};
static const WCHAR prop_servicepackminorW[] =
    {'S','e','r','v','i','c','e','P','a','c','k','M','i','n','o','r','V','e','r','s','i','o','n',0};
static const WCHAR prop_servicetypeW[] =
    {'S','e','r','v','i','c','e','T','y','p','e',0};
static const WCHAR prop_settingidW[] =
    {'S','e','t','t','i','n','g','I','D',0};
static const WCHAR prop_skunumberW[] =
    {'S','K','U','N','u','m','b','e','r',0};
static const WCHAR prop_smbiosbiosversionW[] =
    {'S','M','B','I','O','S','B','I','O','S','V','e','r','s','i','o','n',0};
static const WCHAR prop_smbiosmajorversionW[] =
    {'S','M','B','I','O','S','M','a','j','o','r','V','e','r','s','i','o','n',0};
static const WCHAR prop_smbiosminorversionW[] =
    {'S','M','B','I','O','S','M','i','n','o','r','V','e','r','s','i','o','n',0};
static const WCHAR prop_startmodeW[] =
    {'S','t','a','r','t','M','o','d','e',0};
static const WCHAR prop_sidW[] =
    {'S','I','D',0};
static const WCHAR prop_sidlengthW[] =
    {'S','i','d','L','e','n','g','t','h',0};
static const WCHAR prop_sizeW[] =
    {'S','i','z','e',0};
static const WCHAR prop_speedW[] =
    {'S','p','e','e','d',0};
static const WCHAR prop_startingoffsetW[] =
    {'S','t','a','r','t','i','n','g','O','f','f','s','e','t',0};
static const WCHAR prop_stateW[] =
    {'S','t','a','t','e',0};
static const WCHAR prop_statusW[] =
    {'S','t','a','t','u','s',0};
static const WCHAR prop_statusinfoW[] =
    {'S','t','a','t','u','s','I','n','f','o',0};
static const WCHAR prop_strvalueW[] =
    {'S','t','r','i','n','g','V','a','l','u','e',0};
static const WCHAR prop_suitemaskW[] =
    {'S','u','i','t','e','M','a','s','k',0};
static const WCHAR prop_systemdirectoryW[] =
    {'S','y','s','t','e','m','D','i','r','e','c','t','o','r','y',0};
static const WCHAR prop_systemdriveW[] =
    {'S','y','s','t','e','m','D','r','i','v','e',0};
static const WCHAR prop_systemnameW[] =
    {'S','y','s','t','e','m','N','a','m','e',0};
static const WCHAR prop_tagW[] =
    {'T','a','g',0};
static const WCHAR prop_threadcountW[] =
    {'T','h','r','e','a','d','C','o','u','n','t',0};
static const WCHAR prop_timetakenW[] =
    {'T','i','m','e','T','a','k','e','n',0};
static const WCHAR prop_totalphysicalmemoryW[] =
    {'T','o','t','a','l','P','h','y','s','i','c','a','l','M','e','m','o','r','y',0};
static const WCHAR prop_totalvirtualmemorysizeW[] =
    {'T','o','t','a','l','V','i','r','t','u','a','l','M','e','m','o','r','y','S','i','z','e',0};
static const WCHAR prop_totalvisiblememorysizeW[] =
    {'T','o','t','a','l','V','i','s','i','b','l','e','M','e','m','o','r','y','S','i','z','e',0};
static const WCHAR prop_typeW[] =
    {'T','y','p','e',0};
static const WCHAR prop_uniqueidW[] =
    {'U','n','i','q','u','e','I','d',0};
static const WCHAR prop_usernameW[] =
    {'U','s','e','r','N','a','m','e',0};
static const WCHAR prop_uuidW[] =
    {'U','U','I','D',0};
static const WCHAR prop_vendorW[] =
    {'V','e','n','d','o','r',0};
static const WCHAR prop_versionW[] =
    {'V','e','r','s','i','o','n',0};
static const WCHAR prop_verticalresolutionW[] =
    {'V','e','r','t','i','c','a','l','R','e','s','o','l','u','t','i','o','n',0};
#ifndef __REACTOS__
static const WCHAR prop_videoarchitectureW[] =
    {'V','i','d','e','o','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR prop_videomemorytypeW[] =
    {'V','i','d','e','o','M','e','m','o','r','y','T','y','p','e',0};
static const WCHAR prop_videomodedescriptionW[] =
    {'V','i','d','e','o','M','o','d','e','D','e','s','c','r','i','p','t','i','o','n',0};
static const WCHAR prop_videoprocessorW[] =
    {'V','i','d','e','o','P','r','o','c','e','s','s','o','r',0};
#endif /* !__REACTOS__ */
static const WCHAR prop_volumenameW[] =
    {'V','o','l','u','m','e','N','a','m','e',0};
static const WCHAR prop_volumeserialnumberW[] =
    {'V','o','l','u','m','e','S','e','r','i','a','l','N','u','m','b','e','r',0};
static const WCHAR prop_winsatassessmentstateW[] =
    {'W','i','n','S','A','T','A','s','s','e','s','s','m','e','n','t','S','t','a','t','e',0};
static const WCHAR prop_winsprlevelW[] =
    {'W','i','n','S','P','R','L','e','v','e','l',0};
static const WCHAR prop_workingsetsizeW[] =
    {'W','o','r','k','i','n','g','S','e','t','S','i','z','e',0};

/* column definitions must be kept in sync with record structures below */
static const struct column col_associator[] =
{
    { prop_assocclassW, CIM_STRING },
    { prop_classW,      CIM_STRING },
    { prop_associatorW, CIM_STRING }
};
static const struct column col_baseboard[] =
{
    { prop_manufacturerW,  CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_modelW,         CIM_STRING },
    { prop_nameW,          CIM_STRING },
    { prop_productW,       CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_serialnumberW,  CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_tagW,           CIM_STRING|COL_FLAG_KEY },
    { prop_versionW,       CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_bios[] =
{
    { prop_currentlanguageW,    CIM_STRING },
    { prop_descriptionW,        CIM_STRING },
    { prop_identificationcodeW, CIM_STRING },
    { prop_manufacturerW,       CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_nameW,               CIM_STRING },
    { prop_releasedateW,        CIM_DATETIME|COL_FLAG_DYNAMIC },
    { prop_serialnumberW,       CIM_STRING },
    { prop_smbiosbiosversionW,  CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_smbiosmajorversionW, CIM_UINT16 },
    { prop_smbiosminorversionW, CIM_UINT16 },
    { prop_versionW,            CIM_STRING|COL_FLAG_KEY }
};
static const struct column col_cdromdrive[] =
{
    { prop_deviceidW,    CIM_STRING|COL_FLAG_KEY },
    { prop_driveW,       CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_mediatypeW,   CIM_STRING },
    { prop_nameW,        CIM_STRING },
    { prop_pnpdeviceidW, CIM_STRING }
};
static const struct column col_compsys[] =
{
    { prop_descriptionW,          CIM_STRING },
    { prop_domainW,               CIM_STRING },
    { prop_domainroleW,           CIM_UINT16 },
    { prop_manufacturerW,         CIM_STRING },
    { prop_modelW,                CIM_STRING },
    { prop_nameW,                 CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_numlogicalprocessorsW, CIM_UINT32 },
    { prop_numprocessorsW,        CIM_UINT32 },
    { prop_totalphysicalmemoryW,  CIM_UINT64 },
    { prop_usernameW,             CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_compsysproduct[] =
{
    { prop_identifyingnumberW,  CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_nameW,               CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_skunumberW,          CIM_STRING },
    { prop_uuidW,               CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_vendorW,             CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_versionW,            CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY }
};
static const struct column col_datafile[] =
{
    { prop_nameW,    CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_versionW, CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_desktopmonitor[] =
{
    { prop_pixelsperxlogicalinchW, CIM_UINT32 }
};
static const struct column col_directory[] =
{
    { prop_accessmaskW, CIM_UINT32 },
    { prop_nameW,       CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY }
};
static const struct column col_diskdrive[] =
{
    { prop_deviceidW,      CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_indexW,         CIM_UINT32 },
    { prop_interfacetypeW, CIM_STRING },
    { prop_manufacturerW,  CIM_STRING },
    { prop_mediatypeW,     CIM_STRING },
    { prop_modelW,         CIM_STRING },
    { prop_pnpdeviceidW,   CIM_STRING },
    { prop_serialnumberW,  CIM_STRING },
    { prop_sizeW,          CIM_UINT64 }
};
static const struct column col_diskdrivetodiskpartition[] =
{
    { prop_antecedentW, CIM_REFERENCE|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_dependentW,  CIM_REFERENCE|COL_FLAG_DYNAMIC|COL_FLAG_KEY }
};
static const struct column col_diskpartition[] =
{
    { prop_bootableW,       CIM_BOOLEAN },
    { prop_bootpartitionW,  CIM_BOOLEAN },
    { prop_deviceidW,       CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_diskindexW,      CIM_UINT32 },
    { prop_indexW,          CIM_UINT32 },
    { prop_pnpdeviceidW,    CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_sizeW,           CIM_UINT64 },
    { prop_startingoffsetW, CIM_UINT64 },
    { prop_typeW,           CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_displaycontrollerconfig[] =
{
    { prop_bitsperpixelW,         CIM_UINT32 },
    { prop_captionW,              CIM_STRING },
    { prop_horizontalresolutionW, CIM_UINT32 },
    { prop_nameW,                 CIM_STRING|COL_FLAG_KEY },
    { prop_verticalresolutionW,   CIM_UINT32 }
};
static const struct column col_ip4routetable[] =
{
    { prop_destinationW,    CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_interfaceindexW, CIM_SINT32|COL_FLAG_KEY },
    { prop_nexthopW,        CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
};
static const struct column col_logicaldisk[] =
{
    { prop_deviceidW,           CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_drivetypeW,          CIM_UINT32 },
    { prop_filesystemW,         CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_freespaceW,          CIM_UINT64 },
    { prop_nameW,               CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_sizeW,               CIM_UINT64 },
    { prop_volumenameW,         CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_volumeserialnumberW, CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_logicaldisktopartition[] =
{
    { prop_antecedentW, CIM_REFERENCE|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_dependentW,  CIM_REFERENCE|COL_FLAG_DYNAMIC|COL_FLAG_KEY }
};
static const struct column col_networkadapter[] =
{
    { prop_adaptertypeW,         CIM_STRING },
    { prop_adaptertypeidW,       CIM_UINT16 },
    { prop_descriptionW,         CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_deviceidW,            CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_indexW,               CIM_UINT32 },
    { prop_interfaceindexW,      CIM_UINT32 },
    { prop_macaddressW,          CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_manufacturerW,        CIM_STRING },
    { prop_nameW,                CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_netconnectionstatusW, CIM_UINT16 },
    { prop_physicaladapterW,     CIM_BOOLEAN },
    { prop_pnpdeviceidW,         CIM_STRING },
    { prop_speedW,               CIM_UINT64 }
};
static const struct column col_networkadapterconfig[] =
{
    { prop_defaultipgatewayW,     CIM_STRING|CIM_FLAG_ARRAY|COL_FLAG_DYNAMIC },
    { prop_descriptionW,          CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_dhcpenabledW,          CIM_BOOLEAN },
    { prop_dnshostnameW,          CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_dnsserversearchorderW, CIM_STRING|CIM_FLAG_ARRAY|COL_FLAG_DYNAMIC },
    { prop_indexW,                CIM_UINT32|COL_FLAG_KEY },
    { prop_ipaddressW,            CIM_STRING|CIM_FLAG_ARRAY|COL_FLAG_DYNAMIC },
    { prop_ipconnectionmetricW,   CIM_UINT32 },
    { prop_ipenabledW,            CIM_BOOLEAN },
    { prop_ipsubnet,              CIM_STRING|CIM_FLAG_ARRAY|COL_FLAG_DYNAMIC },
    { prop_macaddressW,           CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_settingidW,            CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_operatingsystem[] =
{
    { prop_buildnumberW,            CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_captionW,                CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_codesetW,                CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_countrycodeW,            CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_csdversionW,             CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_csnameW,                 CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_currenttimezoneW,        CIM_SINT16 },
    { prop_freephysicalmemoryW,     CIM_UINT64 },
    { prop_installdateW,            CIM_DATETIME },
    { prop_lastbootuptimeW,         CIM_DATETIME|COL_FLAG_DYNAMIC },
    { prop_localdatetimeW,          CIM_DATETIME|COL_FLAG_DYNAMIC },
    { prop_localeW,                 CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_manufacturerW,           CIM_STRING },
    { prop_nameW,                   CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_operatingsystemskuW,     CIM_UINT32 },
    { prop_osarchitectureW,         CIM_STRING },
    { prop_oslanguageW,             CIM_UINT32 },
    { prop_osproductsuiteW,         CIM_UINT32 },
    { prop_ostypeW,                 CIM_UINT16 },
    { prop_primaryW,                CIM_BOOLEAN },
    { prop_serialnumberW,           CIM_STRING },
    { prop_servicepackmajorW,       CIM_UINT16 },
    { prop_servicepackminorW,       CIM_UINT16 },
    { prop_suitemaskW,              CIM_UINT32 },
    { prop_systemdirectoryW,        CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_systemdriveW,            CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_totalvirtualmemorysizeW, CIM_UINT64 },
    { prop_totalvisiblememorysizeW, CIM_UINT64 },
    { prop_versionW,                CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_param[] =
{
    { prop_classW,        CIM_STRING },
    { prop_methodW,       CIM_STRING },
    { prop_directionW,    CIM_SINT32 },
    { prop_parameterW,    CIM_STRING },
    { prop_typeW,         CIM_UINT32 },
    { prop_defaultvalueW, CIM_UINT32 }
};
static const struct column col_physicalmedia[] =
{
    { prop_serialnumberW,       CIM_STRING },
    { prop_tagW,                CIM_STRING }
};
static const struct column col_physicalmemory[] =
{
    { prop_capacityW,             CIM_UINT64 },
    { prop_configuredclockspeedW, CIM_UINT32 },
    { prop_devicelocatorW,        CIM_STRING },
    { prop_memorytypeW,           CIM_UINT16 },
    { prop_partnumberW,           CIM_STRING }
};
static const struct column col_pnpentity[] =
{
    { prop_deviceidW, CIM_STRING|COL_FLAG_DYNAMIC },
};
static const struct column col_printer[] =
{
    { prop_attributesW,           CIM_UINT32 },
    { prop_deviceidW,             CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_drivernameW,           CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_horizontalresolutionW, CIM_UINT32 },
    { prop_localW,                CIM_BOOLEAN },
    { prop_locationW,             CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_nameW,                 CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_networkW,              CIM_BOOLEAN },
    { prop_portnameW,             CIM_STRING|COL_FLAG_DYNAMIC },
};
static const struct column col_process[] =
{
    { prop_captionW,        CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_commandlineW,    CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_descriptionW,    CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_handleW,         CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_nameW,           CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_pprocessidW,     CIM_UINT32 },
    { prop_processidW,      CIM_UINT32 },
    { prop_threadcountW,    CIM_UINT32 },
    { prop_workingsetsizeW, CIM_UINT64 },
    /* methods */
    { method_getownerW,     CIM_FLAG_ARRAY|COL_FLAG_METHOD }
};
static const struct column col_processor[] =
{
    { prop_addresswidthW,         CIM_UINT16 },
    { prop_architectureW,         CIM_UINT16 },
    { prop_captionW,              CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_cpustatusW,            CIM_UINT16 },
    { prop_currentclockspeedW,    CIM_UINT32 },
    { prop_datawidthW,            CIM_UINT16 },
    { prop_descriptionW,          CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_deviceidW,             CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_familyW,               CIM_UINT16 },
    { prop_levelW,                CIM_UINT16 },
    { prop_manufacturerW,         CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_maxclockspeedW,        CIM_UINT32 },
    { prop_nameW,                 CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_numcoresW,             CIM_UINT32 },
    { prop_numlogicalprocessorsW, CIM_UINT32 },
    { prop_processoridW,          CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_processortypeW,        CIM_UINT16 },
    { prop_revisionW,             CIM_UINT16 },
    { prop_uniqueidW,             CIM_STRING },
    { prop_versionW,              CIM_STRING|COL_FLAG_DYNAMIC }
};
static const struct column col_qualifier[] =
{
    { prop_classW,     CIM_STRING },
    { prop_memberW,    CIM_STRING },
    { prop_typeW,      CIM_UINT32 },
    { prop_flavorW,    CIM_SINT32 },
    { prop_nameW,      CIM_STRING },
    { prop_intvalueW,  CIM_SINT32 },
    { prop_strvalueW,  CIM_STRING },
    { prop_boolvalueW, CIM_BOOLEAN }
};
static const struct column col_quickfixengineering[] =
{
    { prop_captionW,  CIM_STRING },
    { prop_hotfixidW, CIM_STRING|COL_FLAG_KEY }
};
static const struct column col_service[] =
{
    { prop_acceptpauseW,      CIM_BOOLEAN },
    { prop_acceptstopW,       CIM_BOOLEAN },
    { prop_displaynameW,      CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_nameW,             CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_processidW,        CIM_UINT32 },
    { prop_servicetypeW,      CIM_STRING },
    { prop_startmodeW,        CIM_STRING },
    { prop_stateW,            CIM_STRING },
    { prop_systemnameW,       CIM_STRING|COL_FLAG_DYNAMIC },
    /* methods */
    { method_pauseserviceW,   CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_resumeserviceW,  CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_startserviceW,   CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_stopserviceW,    CIM_FLAG_ARRAY|COL_FLAG_METHOD }
};
static const struct column col_sid[] =
{
    { prop_accountnameW,            CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_binaryrepresentationW,   CIM_UINT8|CIM_FLAG_ARRAY|COL_FLAG_DYNAMIC },
    { prop_referenceddomainnameW,   CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_sidW,                    CIM_STRING|COL_FLAG_DYNAMIC|COL_FLAG_KEY },
    { prop_sidlengthW,              CIM_UINT32 }
};
static const struct column col_sounddevice[] =
{
    { prop_nameW,        CIM_STRING },
    { prop_productnameW, CIM_STRING },
    { prop_statusinfoW,  CIM_UINT16 }
};
static const struct column col_stdregprov[] =
{
    { method_createkeyW,      CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_enumkeyW,        CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_enumvaluesW,     CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_getstringvalueW, CIM_FLAG_ARRAY|COL_FLAG_METHOD }
};
static const struct column col_systemenclosure[] =
{
    { prop_captionW,      CIM_STRING },
    { prop_chassistypesW, CIM_UINT16|CIM_FLAG_ARRAY|COL_FLAG_DYNAMIC },
    { prop_descriptionW,  CIM_STRING },
    { prop_lockpresentW,  CIM_BOOLEAN },
    { prop_manufacturerW, CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_nameW,         CIM_STRING },
    { prop_tagW,          CIM_STRING },
};
static const struct column col_systemsecurity[] =
{
    { method_getsdW,                    CIM_FLAG_ARRAY|COL_FLAG_METHOD },
    { method_setsdW,                    CIM_FLAG_ARRAY|COL_FLAG_METHOD },
};

#ifndef __REACTOS__
static const struct column col_videocontroller[] =
{
    { prop_adapterdactypeW,         CIM_STRING },
    { prop_adapterramW,             CIM_UINT32 },
    { prop_availabilityW,           CIM_UINT16 },
    { prop_captionW,                CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_configmanagererrorcodeW, CIM_UINT32 },
    { prop_currentbitsperpixelW,    CIM_UINT32 },
    { prop_currenthorizontalresW,   CIM_UINT32 },
    { prop_currentrefreshrateW,     CIM_UINT32 },
    { prop_currentscanmodeW,        CIM_UINT16 },
    { prop_currentverticalresW,     CIM_UINT32 },
    { prop_descriptionW,            CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_deviceidW,               CIM_STRING|COL_FLAG_KEY },
    { prop_driverdateW,             CIM_DATETIME },
    { prop_driverversionW,          CIM_STRING },
    { prop_installeddisplaydriversW,CIM_STRING },
    { prop_nameW,                   CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_pnpdeviceidW,            CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_statusW,                 CIM_STRING },
    { prop_videoarchitectureW,      CIM_UINT16 },
    { prop_videomemorytypeW,        CIM_UINT16 },
    { prop_videomodedescriptionW,   CIM_STRING|COL_FLAG_DYNAMIC },
    { prop_videoprocessorW,         CIM_STRING|COL_FLAG_DYNAMIC },
};
#endif

static const struct column col_winsat[] =
{
    { prop_cpuscoreW,              CIM_REAL32 },
    { prop_d3dscoreW,              CIM_REAL32 },
    { prop_diskscoreW,             CIM_REAL32 },
    { prop_graphicsscoreW,         CIM_REAL32 },
    { prop_memoryscoreW,           CIM_REAL32 },
    { prop_timetakenW,             CIM_STRING|COL_FLAG_KEY },
    { prop_winsatassessmentstateW, CIM_UINT32 },
    { prop_winsprlevelW,           CIM_REAL32 },
};


static const WCHAR baseboard_manufacturerW[] =
    {'I','n','t','e','l',' ','C','o','r','p','o','r','a','t','i','o','n',0};
static const WCHAR baseboard_serialnumberW[] =
    {'N','o','n','e',0};
static const WCHAR baseboard_tagW[] =
    {'B','a','s','e',' ','B','o','a','r','d',0};
static const WCHAR baseboard_versionW[] =
    {'1','.','0',0};
static const WCHAR bios_descriptionW[] =
    {'D','e','f','a','u','l','t',' ','S','y','s','t','e','m',' ','B','I','O','S',0};
static const WCHAR bios_manufacturerW[] =
    {'T','h','e',' ','W','i','n','e',' ','P','r','o','j','e','c','t',0};
static const WCHAR bios_releasedateW[] =
    {'2','0','1','2','0','6','0','8','0','0','0','0','0','0','.','0','0','0','0','0','0','+','0','0','0',0};
static const WCHAR bios_serialnumberW[] =
    {'0',0};
static const WCHAR bios_smbiosbiosversionW[] =
    {'W','i','n','e',0};
static const WCHAR bios_versionW[] =
    {'W','I','N','E',' ',' ',' ','-',' ','1',0};
static const WCHAR cdromdrive_mediatypeW[] =
    {'C','D','-','R','O','M',0};
static const WCHAR cdromdrive_nameW[] =
    {'W','i','n','e',' ','C','D','-','R','O','M',' ','A','T','A',' ','D','e','v','i','c','e',0};
static const WCHAR cdromdrive_pnpdeviceidW[]=
    {'I','D','E','\\','C','D','R','O','M','W','I','N','E','_','C','D','-','R','O','M',
     '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
     '_','_','_','_','_','_','_','1','.','0','_','_','_','_','_','\\','5','&','3','A','2',
     'A','5','8','5','4','&','0','&','1','.','0','.','0',0};
static const WCHAR compsys_descriptionW[] =
    {'A','T','/','A','T',' ','C','O','M','P','A','T','I','B','L','E',0};
static const WCHAR compsys_domainW[] =
    {'W','O','R','K','G','R','O','U','P',0};
static const WCHAR compsys_manufacturerW[] =
    {'T','h','e',' ','W','i','n','e',' ','P','r','o','j','e','c','t',0};
static const WCHAR compsys_modelW[] =
    {'W','i','n','e',0};
static const WCHAR compsysproduct_identifyingnumberW[] =
    {'0',0};
static const WCHAR compsysproduct_nameW[] =
    {'W','i','n','e',0};
static const WCHAR compsysproduct_uuidW[] =
    {'d','e','a','d','d','e','a','d','-','d','e','a','d','-','d','e','a','d','-','d','e','a','d','-',
     'd','e','a','d','d','e','a','d','d','e','a','d',0};
static const WCHAR compsysproduct_vendorW[] =
    {'T','h','e',' ','W','i','n','e',' ','P','r','o','j','e','c','t',0};
static const WCHAR compsysproduct_versionW[] =
    {'1','.','0',0};
static const WCHAR diskdrive_interfacetypeW[] =
    {'I','D','E',0};
static const WCHAR diskdrive_manufacturerW[] =
    {'(','S','t','a','n','d','a','r','d',' ','d','i','s','k',' ','d','r','i','v','e','s',')',0};
static const WCHAR diskdrive_mediatype_fixedW[] =
    {'F','i','x','e','d',' ','h','a','r','d',' ','d','i','s','k',0};
static const WCHAR diskdrive_mediatype_removableW[] =
    {'R','e','m','o','v','a','b','l','e',' ','m','e','d','i','a',0};
static const WCHAR diskdrive_modelW[] =
    {'W','i','n','e',' ','D','i','s','k',' ','D','r','i','v','e',0};
static const WCHAR diskdrive_pnpdeviceidW[] =
    {'I','D','E','\\','D','i','s','k','\\','V','E','N','_','W','I','N','E',0};
static const WCHAR diskdrive_serialW[] =
    {'W','I','N','E','H','D','I','S','K',0};
static const WCHAR networkadapter_pnpdeviceidW[]=
    {'P','C','I','\\','V','E','N','_','8','0','8','6','&','D','E','V','_','1','0','0','E','&',
     'S','U','B','S','Y','S','_','0','0','1','E','8','0','8','6','&','R','E','V','_','0','2','\\',
     '3','&','2','6','7','A','6','1','6','A','&','1','&','1','8',0};
static const WCHAR os_32bitW[] =
    {'3','2','-','b','i','t',0};
static const WCHAR os_64bitW[] =
    {'6','4','-','b','i','t',0};
static const WCHAR os_installdateW[] =
    {'2','0','1','4','0','1','0','1','0','0','0','0','0','0','.','0','0','0','0','0','0','+','0','0','0',0};
static const WCHAR os_serialnumberW[] =
    {'1','2','3','4','5','-','O','E','M','-','1','2','3','4','5','6','7','-','1','2','3','4','5',0};
static const WCHAR physicalmedia_tagW[] =
    {'\\','\\','.','\\','P','H','Y','S','I','C','A','L','D','R','I','V','E','0',0};
static const WCHAR quickfixengineering_captionW[] =
    {'h','t','t','p',':','/','/','w','i','n','e','h','q','.','o','r','g',0};
static const WCHAR quickfixengineering_hotfixidW[] =
    {'K','B','1','2','3','4','5','6','7',0};
static const WCHAR sounddevice_productnameW[] =
    {'W','i','n','e',' ','A','u','d','i','o',' ','D','e','v','i','c','e',0};
static const WCHAR systemenclosure_systemenclosureW[] =
    {'S','y','s','t','e','m',' ','E','n','c','l','o','s','u','r','e',0};
static const WCHAR systemenclosure_tagW[] =
    {'S','y','s','t','e','m',' ','E','n','c','l','o','s','u','r','e',' ','0',0};
static const WCHAR systemenclosure_manufacturerW[] =
    {'W','i','n','e',0};
static const WCHAR videocontroller_dactypeW[] =
    {'I','n','t','e','g','r','a','t','e','d',' ','R','A','M','D','A','C',0};
static const WCHAR videocontroller_deviceidW[] =
    {'V','i','d','e','o','C','o','n','t','r','o','l','l','e','r','1',0};
static const WCHAR videocontroller_driverdateW[] =
    {'2','0','1','7','0','1','0','1','0','0','0','0','0','0','.','0','0','0','0','0','0','+','0','0','0',0};
static const WCHAR videocontroller_driverversionW[] =
    {'1','.','0',0};
static const WCHAR videocontroller_statusW[] =
    {'O','K',0};
static const WCHAR winsat_timetakenW[] =
    {'M','o','s','t','R','e','c','e','n','t','A','s','s','e','s','s','m','e','n','t',0};

#include "pshpack1.h"
struct record_associator
{
    const WCHAR *assocclass;
    const WCHAR *class;
    const WCHAR *associator;
};
struct record_baseboard
{
    const WCHAR *manufacturer;
    const WCHAR *model;
    const WCHAR *name;
    const WCHAR *product;
    const WCHAR *serialnumber;
    const WCHAR *tag;
    const WCHAR *version;
};
struct record_bios
{
    const WCHAR *currentlanguage;
    const WCHAR *description;
    const WCHAR *identificationcode;
    const WCHAR *manufacturer;
    const WCHAR *name;
    const WCHAR *releasedate;
    const WCHAR *serialnumber;
    const WCHAR *smbiosbiosversion;
    UINT16       smbiosmajorversion;
    UINT16       smbiosminorversion;
    const WCHAR *version;
};
struct record_cdromdrive
{
    const WCHAR *device_id;
    const WCHAR *drive;
    const WCHAR *mediatype;
    const WCHAR *name;
    const WCHAR *pnpdevice_id;
};
struct record_computersystem
{
    const WCHAR *description;
    const WCHAR *domain;
    UINT16       domainrole;
    const WCHAR *manufacturer;
    const WCHAR *model;
    const WCHAR *name;
    UINT32       num_logical_processors;
    UINT32       num_processors;
    UINT64       total_physical_memory;
    const WCHAR *username;
};
struct record_computersystemproduct
{
    const WCHAR *identifyingnumber;
    const WCHAR *name;
    const WCHAR *skunumber;
    const WCHAR *uuid;
    const WCHAR *vendor;
    const WCHAR *version;
};
struct record_datafile
{
    const WCHAR *name;
    const WCHAR *version;
};
struct record_desktopmonitor
{
    UINT32       pixelsperxlogicalinch;
};
struct record_directory
{
    UINT32       accessmask;
    const WCHAR *name;
};
struct record_diskdrive
{
    const WCHAR *device_id;
    UINT32       index;
    const WCHAR *interfacetype;
    const WCHAR *manufacturer;
    const WCHAR *mediatype;
    const WCHAR *model;
    const WCHAR *pnpdevice_id;
    const WCHAR *serialnumber;
    UINT64       size;
};
struct record_diskdrivetodiskpartition
{
    const WCHAR *antecedent;
    const WCHAR *dependent;
};
struct record_diskpartition
{
    int          bootable;
    int          bootpartition;
    const WCHAR *device_id;
    UINT32       diskindex;
    UINT32       index;
    const WCHAR *pnpdevice_id;
    UINT64       size;
    UINT64       startingoffset;
    const WCHAR *type;
};
struct record_displaycontrollerconfig
{
    UINT32       bitsperpixel;
    const WCHAR *caption;
    UINT32       horizontalresolution;
    const WCHAR *name;
    UINT32       verticalresolution;
};
struct record_ip4routetable
{
    const WCHAR *destination;
    INT32        interfaceindex;
    const WCHAR *nexthop;
};
struct record_logicaldisk
{
    const WCHAR *device_id;
    UINT32       drivetype;
    const WCHAR *filesystem;
    UINT64       freespace;
    const WCHAR *name;
    UINT64       size;
    const WCHAR *volumename;
    const WCHAR *volumeserialnumber;
};
struct record_logicaldisktopartition
{
    const WCHAR *antecedent;
    const WCHAR *dependent;
};
struct record_networkadapter
{
    const WCHAR *adaptertype;
    UINT16       adaptertypeid;
    const WCHAR *description;
    const WCHAR *device_id;
    UINT32       index;
    UINT32       interface_index;
    const WCHAR *mac_address;
    const WCHAR *manufacturer;
    const WCHAR *name;
    UINT16       netconnection_status;
    int          physicaladapter;
    const WCHAR *pnpdevice_id;
    UINT64       speed;
};
struct record_networkadapterconfig
{
    const struct array *defaultipgateway;
    const WCHAR        *description;
    int                 dhcpenabled;
    const WCHAR        *dnshostname;
    const struct array *dnsserversearchorder;
    UINT32              index;
    const struct array *ipaddress;
    UINT32              ipconnectionmetric;
    int                 ipenabled;
    const struct array *ipsubnet;
    const WCHAR        *mac_address;
    const WCHAR        *settingid;
};
struct record_operatingsystem
{
    const WCHAR *buildnumber;
    const WCHAR *caption;
    const WCHAR *codeset;
    const WCHAR *countrycode;
    const WCHAR *csdversion;
    const WCHAR *csname;
    INT16        currenttimezone;
    UINT64       freephysicalmemory;
    const WCHAR *installdate;
    const WCHAR *lastbootuptime;
    const WCHAR *localdatetime;
    const WCHAR *locale;
    const WCHAR *manufacturer;
    const WCHAR *name;
    UINT32       operatingsystemsku;
    const WCHAR *osarchitecture;
    UINT32       oslanguage;
    UINT32       osproductsuite;
    UINT16       ostype;
    int          primary;
    const WCHAR *serialnumber;
    UINT16       servicepackmajor;
    UINT16       servicepackminor;
    UINT32       suitemask;
    const WCHAR *systemdirectory;
    const WCHAR *systemdrive;
    UINT64       totalvirtualmemorysize;
    UINT64       totalvisiblememorysize;
    const WCHAR *version;
};
struct record_param
{
    const WCHAR *class;
    const WCHAR *method;
    INT32        direction;
    const WCHAR *parameter;
    UINT32       type;
    UINT32       defaultvalue;
};
struct record_physicalmedia
{
    const WCHAR *serialnumber;
    const WCHAR *tag;
};
struct record_physicalmemory
{
    UINT64       capacity;
    UINT32       configuredclockspeed;
    const WCHAR *devicelocator;
    UINT16       memorytype;
    const WCHAR *partnumber;
};
struct record_pnpentity
{
    const WCHAR *device_id;
};
struct record_printer
{
    UINT32       attributes;
    const WCHAR *device_id;
    const WCHAR *drivername;
    UINT32       horizontalresolution;
    int          local;
    const WCHAR *location;
    const WCHAR *name;
    int          network;
    const WCHAR *portname;
};
struct record_process
{
    const WCHAR *caption;
    const WCHAR *commandline;
    const WCHAR *description;
    const WCHAR *handle;
    const WCHAR *name;
    UINT32       pprocess_id;
    UINT32       process_id;
    UINT32       thread_count;
    UINT64       workingsetsize;
    /* methods */
    class_method *get_owner;
};
struct record_processor
{
    UINT16       addresswidth;
    UINT16       architecture;
    const WCHAR *caption;
    UINT16       cpu_status;
    UINT32       currentclockspeed;
    UINT16       datawidth;
    const WCHAR *description;
    const WCHAR *device_id;
    UINT16       family;
    UINT16       level;
    const WCHAR *manufacturer;
    UINT32       maxclockspeed;
    const WCHAR *name;
    UINT32       num_cores;
    UINT32       num_logical_processors;
    const WCHAR *processor_id;
    UINT16       processortype;
    UINT16       revision;
    const WCHAR *unique_id;
    const WCHAR *version;
};
struct record_qualifier
{
    const WCHAR *class;
    const WCHAR *member;
    UINT32       type;
    INT32        flavor;
    const WCHAR *name;
    INT32        intvalue;
    const WCHAR *strvalue;
    int          boolvalue;
};
struct record_quickfixengineering
{
    const WCHAR *caption;
    const WCHAR *hotfixid;
};
struct record_service
{
    int          accept_pause;
    int          accept_stop;
    const WCHAR *displayname;
    const WCHAR *name;
    UINT32       process_id;
    const WCHAR *servicetype;
    const WCHAR *startmode;
    const WCHAR *state;
    const WCHAR *systemname;
    /* methods */
    class_method *pause_service;
    class_method *resume_service;
    class_method *start_service;
    class_method *stop_service;
};
struct record_sid
{
    const WCHAR *accountname;
    const struct array *binaryrepresentation;
    const WCHAR *referenceddomainname;
    const WCHAR *sid;
    UINT32       sidlength;
};
struct record_sounddevice
{
    const WCHAR *name;
    const WCHAR *productname;
    UINT16       statusinfo;
};
struct record_stdregprov
{
    class_method *createkey;
    class_method *enumkey;
    class_method *enumvalues;
    class_method *getstringvalue;
};
struct record_systemsecurity
{
    class_method *getsd;
    class_method *setsd;
};
struct record_systemenclosure
{
    const WCHAR        *caption;
    const struct array *chassistypes;
    const WCHAR        *description;
    int                 lockpresent;
    const WCHAR        *manufacturer;
    const WCHAR        *name;
    const WCHAR        *tag;
};
struct record_videocontroller
{
    const WCHAR *adapter_dactype;
    UINT32       adapter_ram;
    UINT16       availability;
    const WCHAR *caption;
    UINT32       config_errorcode;
    UINT32       current_bitsperpixel;
    UINT32       current_horizontalres;
    UINT32       current_refreshrate;
    UINT16       current_scanmode;
    UINT32       current_verticalres;
    const WCHAR *description;
    const WCHAR *device_id;
    const WCHAR *driverdate;
    const WCHAR *driverversion;
    const WCHAR *installeddriver;
    const WCHAR *name;
    const WCHAR *pnpdevice_id;
    const WCHAR *status;
    UINT16       videoarchitecture;
    UINT16       videomemorytype;
    const WCHAR *videomodedescription;
    const WCHAR *videoprocessor;
};
struct record_winsat
{
    FLOAT        cpuscore;
    FLOAT        d3dscore;
    FLOAT        diskscrore;
    FLOAT        graphicsscore;
    FLOAT        memoryscore;
    const WCHAR *timetaken;
    UINT32       winsatassessmentstate;
    FLOAT        winsprlevel;
};
#include "poppack.h"

static const struct record_associator data_associator[] =
{
    { class_diskdrivetodiskpartitionW, class_diskpartitionW, class_diskdriveW },
    { class_logicaldisktopartitionW, class_logicaldiskW, class_diskpartitionW },
};
static const struct record_param data_param[] =
{
    { class_processW, method_getownerW, -1, param_returnvalueW, CIM_UINT32 },
    { class_processW, method_getownerW, -1, param_userW, CIM_STRING },
    { class_processW, method_getownerW, -1, param_domainW, CIM_STRING },
    { class_serviceW, method_pauseserviceW, -1, param_returnvalueW, CIM_UINT32 },
    { class_serviceW, method_resumeserviceW, -1, param_returnvalueW, CIM_UINT32 },
    { class_serviceW, method_startserviceW, -1, param_returnvalueW, CIM_UINT32 },
    { class_serviceW, method_stopserviceW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_createkeyW, 1, param_defkeyW, CIM_SINT32, 0x80000002 },
    { class_stdregprovW, method_createkeyW, 1, param_subkeynameW, CIM_STRING },
    { class_stdregprovW, method_createkeyW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_enumkeyW, 1, param_defkeyW, CIM_SINT32, 0x80000002 },
    { class_stdregprovW, method_enumkeyW, 1, param_subkeynameW, CIM_STRING },
    { class_stdregprovW, method_enumkeyW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_enumkeyW, -1, param_namesW, CIM_STRING|CIM_FLAG_ARRAY },
    { class_stdregprovW, method_enumvaluesW, 1, param_defkeyW, CIM_SINT32, 0x80000002 },
    { class_stdregprovW, method_enumvaluesW, 1, param_subkeynameW, CIM_STRING },
    { class_stdregprovW, method_enumvaluesW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_enumvaluesW, -1, param_namesW, CIM_STRING|CIM_FLAG_ARRAY },
    { class_stdregprovW, method_enumvaluesW, -1, param_typesW, CIM_SINT32|CIM_FLAG_ARRAY },
    { class_stdregprovW, method_getstringvalueW, 1, param_defkeyW, CIM_SINT32, 0x80000002 },
    { class_stdregprovW, method_getstringvalueW, 1, param_subkeynameW, CIM_STRING },
    { class_stdregprovW, method_getstringvalueW, 1, param_valuenameW, CIM_STRING },
    { class_stdregprovW, method_getstringvalueW, -1, param_returnvalueW, CIM_UINT32 },
    { class_stdregprovW, method_getstringvalueW, -1, param_valueW, CIM_STRING },
    { class_systemsecurityW, method_getsdW, -1, param_returnvalueW, CIM_UINT32 },
    { class_systemsecurityW, method_getsdW, -1, param_sdW, CIM_UINT8|CIM_FLAG_ARRAY },
    { class_systemsecurityW, method_setsdW, 1, param_sdW, CIM_UINT8|CIM_FLAG_ARRAY },
    { class_systemsecurityW, method_setsdW, -1, param_returnvalueW, CIM_UINT32 },
};

#define FLAVOR_ID (WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE | WBEM_FLAVOR_NOT_OVERRIDABLE |\
                   WBEM_FLAVOR_ORIGIN_PROPAGATED)

static const struct record_physicalmedia data_physicalmedia[] =
{
    { diskdrive_serialW, physicalmedia_tagW }
};
static const struct record_qualifier data_qualifier[] =
{
    { class_process_getowner_outW, param_userW, CIM_SINT32, FLAVOR_ID, prop_idW, 0 },
    { class_process_getowner_outW, param_domainW, CIM_SINT32, FLAVOR_ID, prop_idW, 1 }
};
static const struct record_quickfixengineering data_quickfixengineering[] =
{
    { quickfixengineering_captionW, quickfixengineering_hotfixidW },
};
static const struct record_sounddevice data_sounddevice[] =
{
    { sounddevice_productnameW, sounddevice_productnameW, 3 /* enabled */ }
};
static const struct record_stdregprov data_stdregprov[] =
{
    { reg_create_key, reg_enum_key, reg_enum_values, reg_get_stringvalue }
};
static UINT16 systemenclosure_chassistypes[] =
{
    1,
};
static const struct array systemenclosure_chassistypes_array =
{
    sizeof(*systemenclosure_chassistypes),
    ARRAY_SIZE(systemenclosure_chassistypes),
    &systemenclosure_chassistypes
};
static const struct record_systemsecurity data_systemsecurity[] =
{
    { security_get_sd, security_set_sd }
};
static const struct record_winsat data_winsat[] =
{
    { 8.0f, 8.0f, 8.0f, 8.0f, 8.0f, winsat_timetakenW, 1 /* Valid */, 8.0f },
};

/* check if row matches condition and update status */
static BOOL match_row( const struct table *table, UINT row, const struct expr *cond, enum fill_status *status )
{
    LONGLONG val;
    UINT type;

    if (!cond)
    {
        *status = FILL_STATUS_UNFILTERED;
        return TRUE;
    }
    if (eval_cond( table, row, cond, &val, &type ) != S_OK)
    {
        *status = FILL_STATUS_FAILED;
        return FALSE;
    }
    *status = FILL_STATUS_FILTERED;
    return val != 0;
}

static BOOL resize_table( struct table *table, UINT row_count, UINT row_size )
{
    if (!table->num_rows_allocated)
    {
        if (!(table->data = heap_alloc( row_count * row_size ))) return FALSE;
        table->num_rows_allocated = row_count;
        return TRUE;
    }
    if (row_count > table->num_rows_allocated)
    {
        BYTE *data;
        UINT count = max( row_count, table->num_rows_allocated * 2 );
        if (!(data = heap_realloc( table->data, count * row_size ))) return FALSE;
        table->data = data;
        table->num_rows_allocated = count;
    }
    return TRUE;
}

#include "pshpack1.h"
struct smbios_prologue
{
    BYTE  calling_method;
    BYTE  major_version;
    BYTE  minor_version;
    BYTE  revision;
    DWORD length;
};

enum smbios_type
{
    SMBIOS_TYPE_BIOS,
    SMBIOS_TYPE_SYSTEM,
    SMBIOS_TYPE_BASEBOARD,
    SMBIOS_TYPE_CHASSIS,
};

struct smbios_header
{
    BYTE type;
    BYTE length;
    WORD handle;
};

struct smbios_baseboard
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 product;
    BYTE                 version;
    BYTE                 serial;
};

struct smbios_bios
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 version;
    WORD                 start;
    BYTE                 date;
    BYTE                 size;
    UINT64               characteristics;
};

struct smbios_chassis
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 type;
    BYTE                 version;
    BYTE                 serial;
    BYTE                 asset_tag;
};

struct smbios_system
{
    struct smbios_header hdr;
    BYTE                 vendor;
    BYTE                 product;
    BYTE                 version;
    BYTE                 serial;
    BYTE                 uuid[16];
};
#include "poppack.h"

#define RSMB (('R' << 24) | ('S' << 16) | ('M' << 8) | 'B')

static const struct smbios_header *find_smbios_entry( enum smbios_type type, const char *buf, UINT len )
{
    const char *ptr, *start;
    const struct smbios_prologue *prologue;
    const struct smbios_header *hdr;

    if (len < sizeof(struct smbios_prologue)) return NULL;
    prologue = (const struct smbios_prologue *)buf;
    if (prologue->length > len - sizeof(*prologue) || prologue->length < sizeof(*hdr)) return NULL;

    start = (const char *)(prologue + 1);
    hdr = (const struct smbios_header *)start;

    for (;;)
    {
        if ((const char *)hdr - start >= prologue->length - sizeof(*hdr)) return NULL;

        if (!hdr->length)
        {
            WARN( "invalid entry\n" );
            return NULL;
        }

        if (hdr->type == type)
        {
            if ((const char *)hdr - start + hdr->length > prologue->length) return NULL;
            break;
        }
        else /* skip other entries and their strings */
        {
            for (ptr = (const char *)hdr + hdr->length; ptr - buf < len && *ptr; ptr++)
            {
                for (; ptr - buf < len; ptr++) if (!*ptr) break;
            }
            if (ptr == (const char *)hdr + hdr->length) ptr++;
            hdr = (const struct smbios_header *)(ptr + 1);
        }
    }

    return hdr;
}

static WCHAR *get_smbios_string( BYTE id, const char *buf, UINT offset, UINT buflen )
{
    const char *ptr = buf + offset;
    UINT i = 0;

    if (!id || offset >= buflen) return NULL;
    for (ptr = buf + offset; ptr - buf < buflen && *ptr; ptr++)
    {
        if (++i == id) return heap_strdupAW( ptr );
        for (; ptr - buf < buflen; ptr++) if (!*ptr) break;
    }
    return NULL;
}

static WCHAR *get_baseboard_string( BYTE id, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_baseboard *baseboard;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_BASEBOARD, buf, len ))) return NULL;

    baseboard = (const struct smbios_baseboard *)hdr;
    offset = (const char *)baseboard - buf + baseboard->hdr.length;
    return get_smbios_string( id, buf, offset, len );
}

static WCHAR *get_baseboard_manufacturer( const char *buf, UINT len )
{
    WCHAR *ret = get_baseboard_string( 1, buf, len );
    if (!ret) return heap_strdupW( baseboard_manufacturerW );
    return ret;
}

static WCHAR *get_baseboard_product( const char *buf, UINT len )
{
    WCHAR *ret = get_baseboard_string( 2, buf, len );
    if (!ret) return heap_strdupW( baseboard_tagW );
    return ret;
}

static WCHAR *get_baseboard_serialnumber( const char *buf, UINT len )
{
    WCHAR *ret = get_baseboard_string( 4, buf, len );
    if (!ret) return heap_strdupW( baseboard_serialnumberW );
    return ret;
}

static WCHAR *get_baseboard_version( const char *buf, UINT len )
{
    WCHAR *ret = get_baseboard_string( 3, buf, len );
    if (!ret) return heap_strdupW( baseboard_versionW );
    return ret;
}

static enum fill_status fill_baseboard( struct table *table, const struct expr *cond )
{
    struct record_baseboard *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0, len;
    char *buf;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    len = GetSystemFirmwareTable( RSMB, 0, NULL, 0 );
    if (!(buf = heap_alloc( len ))) return FILL_STATUS_FAILED;
    GetSystemFirmwareTable( RSMB, 0, buf, len );

    rec = (struct record_baseboard *)table->data;
    rec->manufacturer = get_baseboard_manufacturer( buf, len );
    rec->model        = baseboard_tagW;
    rec->name         = baseboard_tagW;
    rec->product      = get_baseboard_product( buf, len );
    rec->serialnumber = get_baseboard_serialnumber( buf, len );
    rec->tag          = baseboard_tagW;
    rec->version      = get_baseboard_version( buf, len );
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    heap_free( buf );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static UINT16 get_bios_smbiosmajorversion( const char *buf, UINT len )
{
    const struct smbios_prologue *prologue = (const struct smbios_prologue *)buf;
    if (len < sizeof(*prologue)) return 2;
    return prologue->major_version;
}

static UINT16 get_bios_smbiosminorversion( const char *buf, UINT len )
{
    const struct smbios_prologue *prologue = (const struct smbios_prologue *)buf;
    if (len < sizeof(*prologue)) return 0;
    return prologue->minor_version;
}

static WCHAR *get_bios_string( BYTE id, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_bios *bios;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_BIOS, buf, len ))) return NULL;

    bios = (const struct smbios_bios *)hdr;
    offset = (const char *)bios - buf + bios->hdr.length;
    return get_smbios_string( id, buf, offset, len );
}

static WCHAR *get_bios_manufacturer( const char *buf, UINT len )
{
    WCHAR *ret = get_bios_string( 1, buf, len );
    if (!ret) return heap_strdupW( bios_manufacturerW );
    return ret;
}

static WCHAR *convert_bios_date( const WCHAR *str )
{
    static const WCHAR fmtW[] =
        {'%','0','4','u','%','0','2','u','%','0','2','u','0','0','0','0','0','0','.','0','0','0','0','0','0','+','0','0','0',0};
    UINT year, month, day, len = lstrlenW( str );
    const WCHAR *p = str, *q;
    WCHAR *ret;

    while (len && iswspace( *p )) { p++; len--; }
    while (len && iswspace( p[len - 1] )) { len--; }

    q = p;
    while (len && iswdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len || *q != '/') return NULL;
    month = (p[0] - '0') * 10 + p[1] - '0';

    p = ++q; len--;
    while (len && iswdigit( *q )) { q++; len--; };
    if (q - p != 2 || !len || *q != '/') return NULL;
    day = (p[0] - '0') * 10 + p[1] - '0';

    p = ++q; len--;
    while (len && iswdigit( *q )) { q++; len--; };
    if (q - p == 4) year = (p[0] - '0') * 1000 + (p[1] - '0') * 100 + (p[2] - '0') * 10 + p[3] - '0';
    else if (q - p == 2) year = 1900 + (p[0] - '0') * 10 + p[1] - '0';
    else return NULL;

    if (!(ret = heap_alloc( sizeof(fmtW) ))) return NULL;
    swprintf( ret, fmtW, year, month, day );
    return ret;
}

static WCHAR *get_bios_releasedate( const char *buf, UINT len )
{
    WCHAR *ret, *date = get_bios_string( 3, buf, len );
    if (!date || !(ret = convert_bios_date( date ))) ret = heap_strdupW( bios_releasedateW );
    heap_free( date );
    return ret;
}

static WCHAR *get_bios_smbiosbiosversion( const char *buf, UINT len )
{
    WCHAR *ret = get_bios_string( 2, buf, len );
    if (!ret) return heap_strdupW( bios_smbiosbiosversionW );
    return ret;
}

static enum fill_status fill_bios( struct table *table, const struct expr *cond )
{
    struct record_bios *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0, len;
    char *buf;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    len = GetSystemFirmwareTable( RSMB, 0, NULL, 0 );
    if (!(buf = heap_alloc( len ))) return FILL_STATUS_FAILED;
    GetSystemFirmwareTable( RSMB, 0, buf, len );

    rec = (struct record_bios *)table->data;
    rec->currentlanguage    = NULL;
    rec->description        = bios_descriptionW;
    rec->identificationcode = NULL;
    rec->manufacturer       = get_bios_manufacturer( buf, len );
    rec->name               = bios_descriptionW;
    rec->releasedate        = get_bios_releasedate( buf, len );
    rec->serialnumber       = bios_serialnumberW;
    rec->smbiosbiosversion  = get_bios_smbiosbiosversion( buf, len );
    rec->smbiosmajorversion = get_bios_smbiosmajorversion( buf, len );
    rec->smbiosminorversion = get_bios_smbiosminorversion( buf, len );
    rec->version            = bios_versionW;
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    heap_free( buf );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static enum fill_status fill_cdromdrive( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'%','c',':',0};
    WCHAR drive[3], root[] = {'A',':','\\',0};
    struct record_cdromdrive *rec;
    UINT i, row = 0, offset = 0;
    DWORD drives = GetLogicalDrives();
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    for (i = 0; i < 26; i++)
    {
        if (drives & (1 << i))
        {
            root[0] = 'A' + i;
            if (GetDriveTypeW( root ) != DRIVE_CDROM)
                continue;

            if (!resize_table( table, row + 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

            rec = (struct record_cdromdrive *)(table->data + offset);
            rec->device_id    = cdromdrive_pnpdeviceidW;
            swprintf( drive, fmtW, 'A' + i );
            rec->drive        = heap_strdupW( drive );
            rec->mediatype    = cdromdrive_mediatypeW;
            rec->name         = cdromdrive_nameW;
            rec->pnpdevice_id = cdromdrive_pnpdeviceidW;
            if (!match_row( table, row, cond, &status ))
            {
                free_row_values( table, row );
                continue;
            }
            offset += sizeof(*rec);
            row++;
        }
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static UINT get_processor_count(void)
{
    SYSTEM_BASIC_INFORMATION info;

    if (NtQuerySystemInformation( SystemBasicInformation, &info, sizeof(info), NULL )) return 1;
    return info.NumberOfProcessors;
}

#ifdef __REACTOS__
static UINT get_logical_processor_count( UINT *num_cores )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION *info;
    UINT i, j, count = 0;
    NTSTATUS status;
    ULONG len;

    if (num_cores) *num_cores = get_processor_count();
    status = NtQuerySystemInformation( SystemLogicalProcessorInformation, NULL, 0, &len );
    if (status != STATUS_INFO_LENGTH_MISMATCH) return get_processor_count();

    if (!(info = heap_alloc( len ))) return get_processor_count();
    status = NtQuerySystemInformation( SystemLogicalProcessorInformation, info, len, &len );
    if (status != STATUS_SUCCESS)
    {
        heap_free( info );
        return get_processor_count();
    }
    if (num_cores) *num_cores = 0;
    for (i = 0; i < len / sizeof(*info); i++)
    {
        if (info[i].Relationship == RelationProcessorCore)
        {
            for (j = 0; j < sizeof(ULONG_PTR); j++) if (info[i].ProcessorMask & (1 << j)) count++;
        }
        else if (info[i].Relationship == RelationProcessorPackage && num_cores)
        {
            for (j = 0; j < sizeof(ULONG_PTR); j++) if (info[i].ProcessorMask & (1 << j)) (*num_cores)++;
        }
    }
    heap_free( info );
    return count;
}
#else
static UINT get_logical_processor_count( UINT *num_physical, UINT *num_packages )
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *buf, *entry;
    UINT core_relation_count = 0, package_relation_count = 0;
    NTSTATUS status;
    ULONG len, offset = 0;
    BOOL smt_enabled = FALSE;
    DWORD all = RelationAll;

    if (num_packages) *num_packages = 1;
    status = NtQuerySystemInformationEx( SystemLogicalProcessorInformationEx, &all, sizeof(all), NULL, 0, &len );
    if (status != STATUS_INFO_LENGTH_MISMATCH) return get_processor_count();

    if (!(buf = heap_alloc( len ))) return get_processor_count();
    status = NtQuerySystemInformationEx( SystemLogicalProcessorInformationEx, &all, sizeof(all), buf, len, NULL );
    if (status != STATUS_SUCCESS)
    {
        heap_free( buf );
        return get_processor_count();
    }

    while (offset < len)
    {
        entry = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *)((char *)buf + offset);

        if (entry->Relationship == RelationProcessorCore)
        {
            core_relation_count++;
            if (entry->u.Processor.Flags & LTP_PC_SMT) smt_enabled = TRUE;
        }
        else if (entry->Relationship == RelationProcessorPackage)
        {
            package_relation_count++;
        }
        offset += entry->Size;
    }

    heap_free( buf );
    if (num_physical) *num_physical = core_relation_count;
    if (num_packages) *num_packages = package_relation_count;
    return smt_enabled ? core_relation_count * 2 : core_relation_count;
}
#endif

static UINT64 get_total_physical_memory(void)
{
    MEMORYSTATUSEX status;

    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx( &status )) return 1024 * 1024 * 1024;
    return status.ullTotalPhys;
}

static UINT64 get_available_physical_memory(void)
{
    MEMORYSTATUSEX status;

    status.dwLength = sizeof(status);
    if (!GlobalMemoryStatusEx( &status )) return 1024 * 1024 * 1024;
    return status.ullAvailPhys;
}

static WCHAR *get_computername(void)
{
    WCHAR *ret;
    DWORD size = MAX_COMPUTERNAME_LENGTH + 1;

    if (!(ret = heap_alloc( size * sizeof(WCHAR) ))) return NULL;
    GetComputerNameW( ret, &size );
    return ret;
}

static WCHAR *get_username(void)
{
    WCHAR *ret;
    DWORD compsize, usersize;
    DWORD size;

    compsize = 0;
    GetComputerNameW( NULL, &compsize );
    usersize = 0;
    GetUserNameW( NULL, &usersize );
    size = compsize + usersize; /* two null terminators account for the \ */
    if (!(ret = heap_alloc( size * sizeof(WCHAR) ))) return NULL;
    GetComputerNameW( ret, &compsize );
    ret[compsize] = '\\';
    GetUserNameW( ret + compsize + 1, &usersize );
    return ret;
}

static enum fill_status fill_compsys( struct table *table, const struct expr *cond )
{
    struct record_computersystem *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    rec = (struct record_computersystem *)table->data;
    rec->description            = compsys_descriptionW;
    rec->domain                 = compsys_domainW;
    rec->domainrole             = 0; /* standalone workstation */
    rec->manufacturer           = compsys_manufacturerW;
    rec->model                  = compsys_modelW;
    rec->name                   = get_computername();
#ifdef __REACTOS__
    rec->num_logical_processors = get_logical_processor_count( NULL );
    rec->num_processors         = get_processor_count();
#else
    rec->num_logical_processors = get_logical_processor_count( NULL, &rec->num_processors );
#endif
    rec->total_physical_memory  = get_total_physical_memory();
    rec->username               = get_username();
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static WCHAR *get_compsysproduct_string( BYTE id, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_system *system;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_SYSTEM, buf, len ))) return NULL;

    system = (const struct smbios_system *)hdr;
    offset = (const char *)system - buf + system->hdr.length;
    return get_smbios_string( id, buf, offset, len );
}

static WCHAR *get_compsysproduct_identifyingnumber( const char *buf, UINT len )
{
    WCHAR *ret = get_compsysproduct_string( 4, buf, len );
    if (!ret) return heap_strdupW( compsysproduct_identifyingnumberW );
    return ret;
}

static WCHAR *get_compsysproduct_name( const char *buf, UINT len )
{
    WCHAR *ret = get_compsysproduct_string( 2, buf, len );
    if (!ret) return heap_strdupW( compsysproduct_nameW );
    return ret;
}

static WCHAR *get_compsysproduct_uuid( const char *buf, UINT len )
{
    static const WCHAR fmtW[] =
        {'%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','-',
         '%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X','-','%','0','2','X','%','0','2','X',
         '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',0};
    static const BYTE none[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
    const struct smbios_header *hdr;
    const struct smbios_system *system;
    const BYTE *ptr;
    WCHAR *ret = NULL;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_SYSTEM, buf, len )) || hdr->length < sizeof(*system)) goto done;
    system = (const struct smbios_system *)hdr;
    if (!memcmp( system->uuid, none, sizeof(none) ) || !(ret = heap_alloc( 37 * sizeof(WCHAR) ))) goto done;

    ptr = system->uuid;
    swprintf( ret, fmtW, ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], ptr[8], ptr[9],
              ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15] );

done:
    if (!ret) ret = heap_strdupW( compsysproduct_uuidW );
    return ret;
}

static WCHAR *get_compsysproduct_vendor( const char *buf, UINT len )
{
    WCHAR *ret = get_compsysproduct_string( 1, buf, len );
    if (!ret) return heap_strdupW( compsysproduct_vendorW );
    return ret;
}

static WCHAR *get_compsysproduct_version( const char *buf, UINT len )
{
    WCHAR *ret = get_compsysproduct_string( 3, buf, len );
    if (!ret) return heap_strdupW( compsysproduct_versionW );
    return ret;
}

static enum fill_status fill_compsysproduct( struct table *table, const struct expr *cond )
{
    struct record_computersystemproduct *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0, len;
    char *buf;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    len = GetSystemFirmwareTable( RSMB, 0, NULL, 0 );
    if (!(buf = heap_alloc( len ))) return FILL_STATUS_FAILED;
    GetSystemFirmwareTable( RSMB, 0, buf, len );

    rec = (struct record_computersystemproduct *)table->data;
    rec->identifyingnumber = get_compsysproduct_identifyingnumber( buf, len );
    rec->name              = get_compsysproduct_name( buf, len );
    rec->skunumber         = NULL;
    rec->uuid              = get_compsysproduct_uuid( buf, len );
    rec->vendor            = get_compsysproduct_vendor( buf, len );
    rec->version           = get_compsysproduct_version( buf, len );
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    heap_free( buf );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

struct dirstack
{
    WCHAR **dirs;
    UINT   *len_dirs;
    UINT    num_dirs;
    UINT    num_allocated;
};

static struct dirstack *alloc_dirstack( UINT size )
{
    struct dirstack *dirstack;

    if (!(dirstack = heap_alloc( sizeof(*dirstack) ))) return NULL;
    if (!(dirstack->dirs = heap_alloc( sizeof(WCHAR *) * size )))
    {
        heap_free( dirstack );
        return NULL;
    }
    if (!(dirstack->len_dirs = heap_alloc( sizeof(UINT) * size )))
    {
        heap_free( dirstack->dirs );
        heap_free( dirstack );
        return NULL;
    }
    dirstack->num_dirs = 0;
    dirstack->num_allocated = size;
    return dirstack;
}

static void clear_dirstack( struct dirstack *dirstack )
{
    UINT i;
    for (i = 0; i < dirstack->num_dirs; i++) heap_free( dirstack->dirs[i] );
    dirstack->num_dirs = 0;
}

static void free_dirstack( struct dirstack *dirstack )
{
    clear_dirstack( dirstack );
    heap_free( dirstack->dirs );
    heap_free( dirstack->len_dirs );
    heap_free( dirstack );
}

static BOOL push_dir( struct dirstack *dirstack, WCHAR *dir, UINT len )
{
    UINT size, i = dirstack->num_dirs;

    if (!dir) return FALSE;

    if (i == dirstack->num_allocated)
    {
        WCHAR **tmp;
        UINT *len_tmp;

        size = dirstack->num_allocated * 2;
        if (!(tmp = heap_realloc( dirstack->dirs, size * sizeof(WCHAR *) ))) return FALSE;
        dirstack->dirs = tmp;
        if (!(len_tmp = heap_realloc( dirstack->len_dirs, size * sizeof(UINT) ))) return FALSE;
        dirstack->len_dirs = len_tmp;
        dirstack->num_allocated = size;
    }
    dirstack->dirs[i] = dir;
    dirstack->len_dirs[i] = len;
    dirstack->num_dirs++;
    return TRUE;
}

static WCHAR *pop_dir( struct dirstack *dirstack, UINT *len )
{
    if (!dirstack->num_dirs)
    {
        *len = 0;
        return NULL;
    }
    dirstack->num_dirs--;
    *len = dirstack->len_dirs[dirstack->num_dirs];
    return dirstack->dirs[dirstack->num_dirs];
}

static const WCHAR *peek_dir( struct dirstack *dirstack )
{
    if (!dirstack->num_dirs) return NULL;
    return dirstack->dirs[dirstack->num_dirs - 1];
}

static WCHAR *build_glob( WCHAR drive, const WCHAR *path, UINT len )
{
    UINT i = 0;
    WCHAR *ret;

    if (!(ret = heap_alloc( (len + 6) * sizeof(WCHAR) ))) return NULL;
    ret[i++] = drive;
    ret[i++] = ':';
    ret[i++] = '\\';
    if (path && len)
    {
        memcpy( ret + i, path, len * sizeof(WCHAR) );
        i += len;
        ret[i++] = '\\';
    }
    ret[i++] = '*';
    ret[i] = 0;
    return ret;
}

static WCHAR *build_name( WCHAR drive, const WCHAR *path )
{
    UINT i = 0, len = 0;
    const WCHAR *p;
    WCHAR *ret;

    for (p = path; *p; p++)
    {
        if (*p == '\\') len += 2;
        else len++;
    };
    if (!(ret = heap_alloc( (len + 5) * sizeof(WCHAR) ))) return NULL;
    ret[i++] = drive;
    ret[i++] = ':';
    ret[i++] = '\\';
    ret[i++] = '\\';
    for (p = path; *p; p++)
    {
        if (*p != '\\') ret[i++] = *p;
        else
        {
            ret[i++] = '\\';
            ret[i++] = '\\';
        }
    }
    ret[i] = 0;
    return ret;
}

static WCHAR *build_dirname( const WCHAR *path, UINT *ret_len )
{
    const WCHAR *p = path, *start;
    UINT len, i;
    WCHAR *ret;

    if (!iswalpha( p[0] ) || p[1] != ':' || p[2] != '\\' || p[3] != '\\' || !p[4]) return NULL;
    start = path + 4;
    len = lstrlenW( start );
    p = start + len - 1;
    if (*p == '\\') return NULL;

    while (p >= start && *p != '\\') { len--; p--; };
    while (p >= start && *p == '\\') { len--; p--; };

    if (!(ret = heap_alloc( (len + 1) * sizeof(WCHAR) ))) return NULL;
    for (i = 0, p = start; p < start + len; p++)
    {
        if (p[0] == '\\' && p[1] == '\\')
        {
            ret[i++] = '\\';
            p++;
        }
        else ret[i++] = *p;
    }
    ret[i] = 0;
    *ret_len = i;
    return ret;
}

static BOOL seen_dir( struct dirstack *dirstack, const WCHAR *path )
{
    UINT i;
    for (i = 0; i < dirstack->num_dirs; i++) if (!wcscmp( dirstack->dirs[i], path )) return TRUE;
    return FALSE;
}

/* optimize queries of the form WHERE Name='...' [OR Name='...']* */
static UINT seed_dirs( struct dirstack *dirstack, const struct expr *cond, WCHAR root, UINT *count )
{
    const struct expr *left, *right;

    if (!cond || cond->type != EXPR_COMPLEX) return *count = 0;

    left = cond->u.expr.left;
    right = cond->u.expr.right;
    if (cond->u.expr.op == OP_EQ)
    {
        UINT len;
        WCHAR *path;
        const WCHAR *str = NULL;

        if (left->type == EXPR_PROPVAL && right->type == EXPR_SVAL &&
            !wcscmp( left->u.propval->name, prop_nameW ) &&
            towupper( right->u.sval[0] ) == towupper( root ))
        {
            str = right->u.sval;
        }
        else if (left->type == EXPR_SVAL && right->type == EXPR_PROPVAL &&
                 !wcscmp( right->u.propval->name, prop_nameW ) &&
                 towupper( left->u.sval[0] ) == towupper( root ))
        {
            str = left->u.sval;
        }
        if (str && (path = build_dirname( str, &len )))
        {
            if (seen_dir( dirstack, path ))
            {
                heap_free( path );
                return ++*count;
            }
            else if (push_dir( dirstack, path, len )) return ++*count;
            heap_free( path );
            return *count = 0;
        }
    }
    else if (cond->u.expr.op == OP_OR)
    {
        UINT left_count = 0, right_count = 0;

        if (!(seed_dirs( dirstack, left, root, &left_count ))) return *count = 0;
        if (!(seed_dirs( dirstack, right, root, &right_count ))) return *count = 0;
        return *count += left_count + right_count;
    }
    return *count = 0;
}

static WCHAR *append_path( const WCHAR *path, const WCHAR *segment, UINT *len )
{
    UINT len_path = 0, len_segment = lstrlenW( segment );
    WCHAR *ret;

    *len = 0;
    if (path) len_path = lstrlenW( path );
    if (!(ret = heap_alloc( (len_path + len_segment + 2) * sizeof(WCHAR) ))) return NULL;
    if (path && len_path)
    {
        memcpy( ret, path, len_path * sizeof(WCHAR) );
        ret[len_path] = '\\';
        *len += len_path + 1;
    }
    memcpy( ret + *len, segment, len_segment * sizeof(WCHAR) );
    *len += len_segment;
    ret[*len] = 0;
    return ret;
}

static WCHAR *get_file_version( const WCHAR *filename )
{
    static const WCHAR slashW[] = {'\\',0}, fmtW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};
    VS_FIXEDFILEINFO *info;
    DWORD size, len = 4 * 5 + ARRAY_SIZE( fmtW );
    void *block;
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) ))) return NULL;
    if (!(size = GetFileVersionInfoSizeW( filename, NULL )) || !(block = heap_alloc( size )))
    {
        heap_free( ret );
        return NULL;
    }
    if (!GetFileVersionInfoW( filename, 0, size, block ) ||
        !VerQueryValueW( block, slashW, (void **)&info, &size ))
    {
        heap_free( block );
        heap_free( ret );
        return NULL;
    }
    swprintf( ret, fmtW, info->dwFileVersionMS >> 16, info->dwFileVersionMS & 0xffff,
                              info->dwFileVersionLS >> 16, info->dwFileVersionLS & 0xffff );
    heap_free( block );
    return ret;
}

static enum fill_status fill_datafile( struct table *table, const struct expr *cond )
{
    static const WCHAR dotW[] = {'.',0}, dotdotW[] = {'.','.',0};
    struct record_datafile *rec;
    UINT i, len, row = 0, offset = 0, num_expected_rows;
    WCHAR *glob = NULL, *path = NULL, *new_path, root[] = {'A',':','\\',0};
    DWORD drives = GetLogicalDrives();
    WIN32_FIND_DATAW data;
    HANDLE handle;
    struct dirstack *dirstack;
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 8, sizeof(*rec) )) return FILL_STATUS_FAILED;

    dirstack = alloc_dirstack(2);

    for (i = 0; i < 26; i++)
    {
        if (!(drives & (1 << i))) continue;

        root[0] = 'A' + i;
        if (GetDriveTypeW( root ) != DRIVE_FIXED) continue;

        num_expected_rows = 0;
        if (!seed_dirs( dirstack, cond, root[0], &num_expected_rows )) clear_dirstack( dirstack );

        for (;;)
        {
            heap_free( glob );
            heap_free( path );
            path = pop_dir( dirstack, &len );
            if (!(glob = build_glob( root[0], path, len )))
            {
                status = FILL_STATUS_FAILED;
                goto done;
            }
            if ((handle = FindFirstFileW( glob, &data )) != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (!resize_table( table, row + 1, sizeof(*rec) ))
                    {
                        status = FILL_STATUS_FAILED;
                        FindClose( handle );
                        goto done;
                    }
                    if (!wcscmp( data.cFileName, dotW ) || !wcscmp( data.cFileName, dotdotW )) continue;

                    if (!(new_path = append_path( path, data.cFileName, &len )))
                    {
                        status = FILL_STATUS_FAILED;
                        FindClose( handle );
                        goto done;
                    }

                    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (push_dir( dirstack, new_path, len )) continue;
                        heap_free( new_path );
                        FindClose( handle );
                        status = FILL_STATUS_FAILED;
                        goto done;
                    }
                    rec = (struct record_datafile *)(table->data + offset);
                    rec->name    = build_name( root[0], new_path );
                    rec->version = get_file_version( rec->name );
                    heap_free( new_path );
                    if (!match_row( table, row, cond, &status ))
                    {
                        free_row_values( table, row );
                        continue;
                    }
                    else if (num_expected_rows && row == num_expected_rows - 1)
                    {
                        row++;
                        FindClose( handle );
                        status = FILL_STATUS_FILTERED;
                        goto done;
                    }
                    offset += sizeof(*rec);
                    row++;
                }
                while (FindNextFileW( handle, &data ));
                FindClose( handle );
            }
            if (!peek_dir( dirstack )) break;
        }
    }

done:
    free_dirstack( dirstack );
    heap_free( glob );
    heap_free( path );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static UINT32 get_pixelsperxlogicalinch(void)
{
    HDC hdc = GetDC( NULL );
    UINT32 ret;

    if (!hdc) return 96;
    ret = GetDeviceCaps( hdc, LOGPIXELSX );
    ReleaseDC( NULL, hdc );
    return ret;
}

static enum fill_status fill_desktopmonitor( struct table *table, const struct expr *cond )
{
    struct record_desktopmonitor *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    rec = (struct record_desktopmonitor *)table->data;
    rec->pixelsperxlogicalinch = get_pixelsperxlogicalinch();

    if (match_row( table, row, cond, &status )) row++;

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static enum fill_status fill_directory( struct table *table, const struct expr *cond )
{
    static const WCHAR dotW[] = {'.',0}, dotdotW[] = {'.','.',0};
    struct record_directory *rec;
    UINT i, len, row = 0, offset = 0, num_expected_rows;
    WCHAR *glob = NULL, *path = NULL, *new_path, root[] = {'A',':','\\',0};
    DWORD drives = GetLogicalDrives();
    WIN32_FIND_DATAW data;
    HANDLE handle;
    struct dirstack *dirstack;
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 4, sizeof(*rec) )) return FILL_STATUS_FAILED;

    dirstack = alloc_dirstack(2);

    for (i = 0; i < 26; i++)
    {
        if (!(drives & (1 << i))) continue;

        root[0] = 'A' + i;
        if (GetDriveTypeW( root ) != DRIVE_FIXED) continue;

        num_expected_rows = 0;
        if (!seed_dirs( dirstack, cond, root[0], &num_expected_rows )) clear_dirstack( dirstack );

        for (;;)
        {
            heap_free( glob );
            heap_free( path );
            path = pop_dir( dirstack, &len );
            if (!(glob = build_glob( root[0], path, len )))
            {
                status = FILL_STATUS_FAILED;
                goto done;
            }
            if ((handle = FindFirstFileW( glob, &data )) != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (!resize_table( table, row + 1, sizeof(*rec) ))
                    {
                        FindClose( handle );
                        status = FILL_STATUS_FAILED;
                        goto done;
                    }
                    if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                        !wcscmp( data.cFileName, dotW ) || !wcscmp( data.cFileName, dotdotW ))
                        continue;

                    if (!(new_path = append_path( path, data.cFileName, &len )))
                    {
                        FindClose( handle );
                        status = FILL_STATUS_FAILED;
                        goto done;
                    }

                    if (!(push_dir( dirstack, new_path, len )))
                    {
                        heap_free( new_path );
                        FindClose( handle );
                        status = FILL_STATUS_FAILED;
                        goto done;
                    }
                    rec = (struct record_directory *)(table->data + offset);
                    rec->accessmask = FILE_ALL_ACCESS;
                    rec->name       = build_name( root[0], new_path );
                    heap_free( new_path );
                    if (!match_row( table, row, cond, &status ))
                    {
                        free_row_values( table, row );
                        continue;
                    }
                    else if (num_expected_rows && row == num_expected_rows - 1)
                    {
                        row++;
                        FindClose( handle );
                        status = FILL_STATUS_FILTERED;
                        goto done;
                    }
                    offset += sizeof(*rec);
                    row++;
                }
                while (FindNextFileW( handle, &data ));
                FindClose( handle );
            }
            if (!peek_dir( dirstack )) break;
        }
    }

done:
    free_dirstack( dirstack );
    heap_free( glob );
    heap_free( path );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static UINT64 get_freespace( const WCHAR *dir, UINT64 *disksize )
{
    WCHAR root[] = {'\\','\\','.','\\','A',':',0};
    ULARGE_INTEGER free;
    DISK_GEOMETRY_EX info;
    HANDLE handle;
    DWORD bytes_returned;

    free.QuadPart = 512 * 1024 * 1024;
    GetDiskFreeSpaceExW( dir, NULL, NULL, &free );

    root[4] = dir[0];
    handle = CreateFileW( root, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (handle != INVALID_HANDLE_VALUE)
    {
        if (DeviceIoControl( handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &info, sizeof(info), &bytes_returned, NULL ))
            *disksize = info.DiskSize.QuadPart;
        CloseHandle( handle );
    }
    return free.QuadPart;
}

static enum fill_status fill_diskdrive( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] =
        {'\\','\\','\\','\\','.','\\','\\','P','H','Y','S','I','C','A','L','D','R','I','V','E','%','u',0};
    WCHAR device_id[ARRAY_SIZE( fmtW ) + 10], root[] = {'A',':','\\',0};
    struct record_diskdrive *rec;
    UINT i, row = 0, offset = 0, index = 0, type;
    UINT64 size = 1024 * 1024 * 1024;
    DWORD drives = GetLogicalDrives();
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 2, sizeof(*rec) )) return FILL_STATUS_FAILED;

    for (i = 0; i < 26; i++)
    {
        if (drives & (1 << i))
        {
            root[0] = 'A' + i;
            type = GetDriveTypeW( root );
            if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE)
                continue;

            if (!resize_table( table, row + 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

            rec = (struct record_diskdrive *)(table->data + offset);
            swprintf( device_id, fmtW, index );
            rec->device_id     = heap_strdupW( device_id );
            rec->index         = index++;
            rec->interfacetype = diskdrive_interfacetypeW;
            rec->manufacturer  = diskdrive_manufacturerW;
            rec->mediatype     = (type == DRIVE_FIXED) ? diskdrive_mediatype_fixedW : diskdrive_mediatype_removableW;
            rec->model         = diskdrive_modelW;
            rec->pnpdevice_id  = diskdrive_pnpdeviceidW;
            rec->serialnumber  = diskdrive_serialW;
            get_freespace( root, &size );
            rec->size          = size;
            if (!match_row( table, row, cond, &status ))
            {
                free_row_values( table, row );
                continue;
            }
            offset += sizeof(*rec);
            row++;
        }
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

struct association
{
    WCHAR *ref;
    WCHAR *ref2;
};

static void free_assocations( struct association *assoc, UINT count )
{
    UINT i;
    if (!assoc) return;
    for (i = 0; i < count; i++)
    {
        heap_free( assoc[i].ref );
        heap_free( assoc[i].ref2 );
    }
    heap_free( assoc );
}

static struct association *get_diskdrivetodiskpartition_pairs( UINT *count )
{
    static const WCHAR pathW[] =
        {'_','_','P','A','T','H',0};
    static const WCHAR selectW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','i','s','k','D','r','i','v','e',0};
    static const WCHAR select2W[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','i','s','k','P','a','r','t','i','t','i','o','n',0};
    struct association *ret = NULL;
    struct query *query, *query2 = NULL;
    VARIANT val;
    HRESULT hr;
    UINT i;

    if (!(query = create_query())) return NULL;
    if ((hr = parse_query( selectW, &query->view, &query->mem )) != S_OK) goto done;
    if ((hr = execute_view( query->view )) != S_OK) goto done;

    if (!(query2 = create_query())) return FALSE;
    if ((hr = parse_query( select2W, &query2->view, &query2->mem )) != S_OK) goto done;
    if ((hr = execute_view( query2->view )) != S_OK) goto done;

    if (!(ret = heap_alloc_zero( query->view->result_count * sizeof(*ret) ))) goto done;

    for (i = 0; i < query->view->result_count; i++)
    {
        if ((hr = get_propval( query->view, i, pathW, &val, NULL, NULL )) != S_OK) goto done;
        if (!(ret[i].ref = heap_strdupW( V_BSTR(&val) ))) goto done;
        VariantClear( &val );

        if ((hr = get_propval( query2->view, i, pathW, &val, NULL, NULL )) != S_OK) goto done;
        if (!(ret[i].ref2 = heap_strdupW( V_BSTR(&val) ))) goto done;
        VariantClear( &val );
    }

    *count = query->view->result_count;

done:
    if (!ret) free_assocations( ret, query->view->result_count );
    free_query( query );
    free_query( query2 );
    return ret;
}

static enum fill_status fill_diskdrivetodiskpartition( struct table *table, const struct expr *cond )
{
    struct record_diskdrivetodiskpartition *rec;
    UINT i, row = 0, offset = 0, count = 0;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    struct association *assoc;

    if (!(assoc = get_diskdrivetodiskpartition_pairs( &count ))) return FILL_STATUS_FAILED;
    if (!count)
    {
        free_assocations( assoc, count );
        return FILL_STATUS_UNFILTERED;
    }
    if (!resize_table( table, count, sizeof(*rec) ))
    {
        free_assocations( assoc, count );
        return FILL_STATUS_FAILED;
    }

    for (i = 0; i < count; i++)
    {
        rec = (struct record_diskdrivetodiskpartition *)(table->data + offset);
        rec->antecedent = assoc[i].ref;
        rec->dependent  = assoc[i].ref2;
        if (!match_row( table, row, cond, &status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    }

    heap_free( assoc );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static WCHAR *get_filesystem( const WCHAR *root )
{
    static const WCHAR ntfsW[] = {'N','T','F','S',0};
    WCHAR buffer[MAX_PATH + 1];

    if (GetVolumeInformationW( root, NULL, 0, NULL, NULL, NULL, buffer, MAX_PATH + 1 ))
        return heap_strdupW( buffer );
    return heap_strdupW( ntfsW );
}

static enum fill_status fill_diskpartition( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] =
        {'D','i','s','k',' ','#','%','u',',',' ','P','a','r','t','i','t','i','o','n',' ','#','0',0};
    WCHAR device_id[32], root[] = {'A',':','\\',0};
    struct record_diskpartition *rec;
    UINT i, row = 0, offset = 0, type, index = 0;
    UINT64 size = 1024 * 1024 * 1024;
    DWORD drives = GetLogicalDrives();
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 4, sizeof(*rec) )) return FILL_STATUS_FAILED;

    for (i = 0; i < 26; i++)
    {
        if (drives & (1 << i))
        {
            root[0] = 'A' + i;
            type = GetDriveTypeW( root );
            if (type != DRIVE_FIXED && type != DRIVE_REMOVABLE)
                continue;

            if (!resize_table( table, row + 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

            rec = (struct record_diskpartition *)(table->data + offset);
            rec->bootable       = (i == 2) ? -1 : 0;
            rec->bootpartition  = (i == 2) ? -1 : 0;
            swprintf( device_id, fmtW, index );
            rec->device_id      = heap_strdupW( device_id );
            rec->diskindex      = index++;
            rec->index          = 0;
            rec->pnpdevice_id   = heap_strdupW( device_id );
            get_freespace( root, &size );
            rec->size           = size;
            rec->startingoffset = 0;
            rec->type           = get_filesystem( root );
            if (!match_row( table, row, cond, &status ))
            {
                free_row_values( table, row );
                continue;
            }
            offset += sizeof(*rec);
            row++;
        }
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static UINT32 get_bitsperpixel( UINT *hres, UINT *vres )
{
    HDC hdc = GetDC( NULL );
    UINT32 ret;

    if (!hdc) return 32;
    ret = GetDeviceCaps( hdc, BITSPIXEL );
    *hres = GetDeviceCaps( hdc, HORZRES );
    *vres = GetDeviceCaps( hdc, VERTRES );
    ReleaseDC( NULL, hdc );
    return ret;
}

static enum fill_status fill_displaycontrollerconfig( struct table *table, const struct expr *cond )
{
    struct record_displaycontrollerconfig *rec;
    UINT row = 0, hres = 1024, vres = 768;
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    rec = (struct record_displaycontrollerconfig *)table->data;
    rec->bitsperpixel         = get_bitsperpixel( &hres, &vres );
    rec->caption              = videocontroller_deviceidW;
    rec->horizontalresolution = hres;
    rec->name                 = videocontroller_deviceidW;
    rec->verticalresolution   = vres;
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static WCHAR *get_ip4_string( DWORD addr )
{
    static const WCHAR fmtW[] = {'%','u','.','%','u','.','%','u','.','%','u',0};
    DWORD len = sizeof("ddd.ddd.ddd.ddd");
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) ))) return NULL;
    swprintf( ret, fmtW, (addr >> 24) & 0xff, (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff );
    return ret;
}

static enum fill_status fill_ip4routetable( struct table *table, const struct expr *cond )
{
    struct record_ip4routetable *rec;
    UINT i, row = 0, offset = 0, size = 0;
    MIB_IPFORWARDTABLE *forwards;
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (GetIpForwardTable( NULL, &size, TRUE ) != ERROR_INSUFFICIENT_BUFFER) return FILL_STATUS_FAILED;
    if (!(forwards = heap_alloc( size ))) return FILL_STATUS_FAILED;
    if (GetIpForwardTable( forwards, &size, TRUE ))
    {
        heap_free( forwards );
        return FILL_STATUS_FAILED;
    }
    if (!resize_table( table, max(forwards->dwNumEntries, 1), sizeof(*rec) ))
    {
        heap_free( forwards );
        return FILL_STATUS_FAILED;
    }

    for (i = 0; i < forwards->dwNumEntries; i++)
    {
        rec = (struct record_ip4routetable *)(table->data + offset);

        rec->destination    = get_ip4_string( ntohl(forwards->table[i].dwForwardDest) );
        rec->interfaceindex = forwards->table[i].dwForwardIfIndex;
        rec->nexthop        = get_ip4_string( ntohl(forwards->table[i].dwForwardNextHop) );

        if (!match_row( table, row, cond, &status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;

    heap_free( forwards );
    return status;
}

static WCHAR *get_volumename( const WCHAR *root )
{
    WCHAR buf[MAX_PATH + 1] = {0};
    GetVolumeInformationW( root, buf, ARRAY_SIZE( buf ), NULL, NULL, NULL, NULL, 0 );
    return heap_strdupW( buf );
}
static WCHAR *get_volumeserialnumber( const WCHAR *root )
{
    static const WCHAR fmtW[] = {'%','0','8','X',0};
    DWORD serial = 0;
    WCHAR buffer[9];

    GetVolumeInformationW( root, NULL, 0, &serial, NULL, NULL, NULL, 0 );
    swprintf( buffer, fmtW, serial );
    return heap_strdupW( buffer );
}

static enum fill_status fill_logicaldisk( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'%','c',':',0};
    WCHAR device_id[3], root[] = {'A',':','\\',0};
    struct record_logicaldisk *rec;
    UINT i, row = 0, offset = 0, type;
    UINT64 size = 1024 * 1024 * 1024;
    DWORD drives = GetLogicalDrives();
    enum fill_status status = FILL_STATUS_UNFILTERED;

    if (!resize_table( table, 4, sizeof(*rec) )) return FILL_STATUS_FAILED;

    for (i = 0; i < 26; i++)
    {
        if (drives & (1 << i))
        {
            root[0] = 'A' + i;
            type = GetDriveTypeW( root );
            if (type != DRIVE_FIXED && type != DRIVE_CDROM && type != DRIVE_REMOVABLE)
                continue;

            if (!resize_table( table, row + 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

            rec = (struct record_logicaldisk *)(table->data + offset);
            swprintf( device_id, fmtW, 'A' + i );
            rec->device_id          = heap_strdupW( device_id );
            rec->drivetype          = type;
            rec->filesystem         = get_filesystem( root );
            rec->freespace          = get_freespace( root, &size );
            rec->name               = heap_strdupW( device_id );
            rec->size               = size;
            rec->volumename         = get_volumename( root );
            rec->volumeserialnumber = get_volumeserialnumber( root );
            if (!match_row( table, row, cond, &status ))
            {
                free_row_values( table, row );
                continue;
            }
            offset += sizeof(*rec);
            row++;
        }
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static struct association *get_logicaldisktopartition_pairs( UINT *count )
{
    static const WCHAR pathW[] =
        {'_','_','P','A','T','H',0};
    static const WCHAR selectW[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'D','i','s','k','P','a','r','t','i','t','i','o','n',0};
    static const WCHAR select2W[] =
        {'S','E','L','E','C','T',' ','*',' ','F','R','O','M',' ','W','i','n','3','2','_',
         'L','o','g','i','c','a','l','D','i','s','k',' ','W','H','E','R','E',' ',
         'D','r','i','v','e','T','y','p','e','=','2',' ','O','R',' ','D','r','i','v','e','T','y','p','e','=','3',0};
    struct association *ret = NULL;
    struct query *query, *query2 = NULL;
    VARIANT val;
    HRESULT hr;
    UINT i;

    if (!(query = create_query())) return NULL;
    if ((hr = parse_query( selectW, &query->view, &query->mem )) != S_OK) goto done;
    if ((hr = execute_view( query->view )) != S_OK) goto done;

    if (!(query2 = create_query())) return FALSE;
    if ((hr = parse_query( select2W, &query2->view, &query2->mem )) != S_OK) goto done;
    if ((hr = execute_view( query2->view )) != S_OK) goto done;

    if (!(ret = heap_alloc_zero( query->view->result_count * sizeof(*ret) ))) goto done;

    /* assume fixed and removable disks are enumerated in the same order as partitions */
    for (i = 0; i < query->view->result_count; i++)
    {
        if ((hr = get_propval( query->view, i, pathW, &val, NULL, NULL )) != S_OK) goto done;
        if (!(ret[i].ref = heap_strdupW( V_BSTR(&val) ))) goto done;
        VariantClear( &val );

        if ((hr = get_propval( query2->view, i, pathW, &val, NULL, NULL )) != S_OK) goto done;
        if (!(ret[i].ref2 = heap_strdupW( V_BSTR(&val) ))) goto done;
        VariantClear( &val );
    }

    *count = query->view->result_count;

done:
    if (!ret) free_assocations( ret, query->view->result_count );
    free_query( query );
    free_query( query2 );
    return ret;
}

static enum fill_status fill_logicaldisktopartition( struct table *table, const struct expr *cond )
{
    struct record_logicaldisktopartition *rec;
    UINT i, row = 0, offset = 0, count = 0;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    struct association *assoc;

    if (!(assoc = get_logicaldisktopartition_pairs( &count ))) return FILL_STATUS_FAILED;
    if (!count)
    {
        free_assocations( assoc, count );
        return FILL_STATUS_UNFILTERED;
    }
    if (!resize_table( table, count, sizeof(*rec) ))
    {
        free_assocations( assoc, count );
        return FILL_STATUS_FAILED;
    }

    for (i = 0; i < count; i++)
    {
        rec = (struct record_logicaldisktopartition *)(table->data + offset);
        rec->antecedent = assoc[i].ref;
        rec->dependent  = assoc[i].ref2;
        if (!match_row( table, row, cond, &status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    }

    heap_free( assoc );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static UINT16 get_connection_status( IF_OPER_STATUS status )
{
    switch (status)
    {
    case IfOperStatusDown:
        return 0; /* Disconnected */
    case IfOperStatusUp:
        return 2; /* Connected */
    default:
        ERR("unhandled status %u\n", status);
        break;
    }
    return 0;
}
static WCHAR *get_mac_address( const BYTE *addr, DWORD len )
{
    static const WCHAR fmtW[] =
        {'%','0','2','x',':','%','0','2','x',':','%','0','2','x',':',
         '%','0','2','x',':','%','0','2','x',':','%','0','2','x',0};
    WCHAR *ret;

    if (len != 6 || !(ret = heap_alloc( 18 * sizeof(WCHAR) ))) return NULL;
    swprintf( ret, fmtW, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5] );
    return ret;
}
static const WCHAR *get_adaptertype( DWORD type, int *id, int *physical )
{
    static const WCHAR ethernetW[] = {'E','t','h','e','r','n','e','t',' ','8','0','2','.','3',0};
    static const WCHAR wirelessW[] = {'W','i','r','e','l','e','s','s',0};
    static const WCHAR firewireW[] = {'1','3','9','4',0};
    static const WCHAR tunnelW[]   = {'T','u','n','n','e','l',0};

    switch (type)
    {
    case IF_TYPE_ETHERNET_CSMACD:
        *id = 0;
        *physical = -1;
        return ethernetW;
    case IF_TYPE_IEEE80211:
        *id = 9;
        *physical = -1;
        return wirelessW;
    case IF_TYPE_IEEE1394:
        *id = 13;
        *physical = -1;
        return firewireW;
    case IF_TYPE_TUNNEL:
        *id = 15;
        *physical = 0;
        return tunnelW;
    default:
        *id = -1;
        *physical = 0;
        return NULL;
    }
}

static enum fill_status fill_networkadapter( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'%','u',0};
    WCHAR device_id[11];
    struct record_networkadapter *rec;
    IP_ADAPTER_ADDRESSES *aa, *buffer;
    UINT row = 0, offset = 0, count = 0;
    DWORD size = 0, ret;
    int adaptertypeid, physical;
    enum fill_status status = FILL_STATUS_UNFILTERED;

    ret = GetAdaptersAddresses( AF_UNSPEC, 0, NULL, NULL, &size );
    if (ret != ERROR_BUFFER_OVERFLOW) return FILL_STATUS_FAILED;

    if (!(buffer = heap_alloc( size ))) return FILL_STATUS_FAILED;
    if (GetAdaptersAddresses( AF_UNSPEC, 0, NULL, buffer, &size ))
    {
        heap_free( buffer );
        return FILL_STATUS_FAILED;
    }
    for (aa = buffer; aa; aa = aa->Next)
    {
        if (aa->IfType != IF_TYPE_SOFTWARE_LOOPBACK) count++;
    }
    if (!resize_table( table, count, sizeof(*rec) ))
    {
        heap_free( buffer );
        return FILL_STATUS_FAILED;
    }
    for (aa = buffer; aa; aa = aa->Next)
    {
        if (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        rec = (struct record_networkadapter *)(table->data + offset);
        swprintf( device_id, fmtW, aa->u.s.IfIndex );
        rec->adaptertype          = get_adaptertype( aa->IfType, &adaptertypeid, &physical );
        rec->adaptertypeid        = adaptertypeid;
        rec->description          = heap_strdupW( aa->Description );
        rec->device_id            = heap_strdupW( device_id );
        rec->index                = aa->u.s.IfIndex;
        rec->interface_index      = aa->u.s.IfIndex;
        rec->mac_address          = get_mac_address( aa->PhysicalAddress, aa->PhysicalAddressLength );
        rec->manufacturer         = compsys_manufacturerW;
        rec->name                 = heap_strdupW( aa->FriendlyName );
        rec->netconnection_status = get_connection_status( aa->OperStatus );
        rec->physicaladapter      = physical;
        rec->pnpdevice_id         = networkadapter_pnpdeviceidW;
        rec->speed                = 1000000;
        if (!match_row( table, row, cond, &status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;

    heap_free( buffer );
    return status;
}

static WCHAR *get_dnshostname( IP_ADAPTER_UNICAST_ADDRESS *addr )
{
    const SOCKET_ADDRESS *sa = &addr->Address;
    WCHAR buf[NI_MAXHOST];

    if (!addr) return NULL;
    if (GetNameInfoW( sa->lpSockaddr, sa->iSockaddrLength, buf, ARRAY_SIZE( buf ), NULL,
                      0, NI_NAMEREQD )) return NULL;
    return heap_strdupW( buf );
}
static struct array *get_defaultipgateway( IP_ADAPTER_GATEWAY_ADDRESS *list )
{
    IP_ADAPTER_GATEWAY_ADDRESS *gateway;
    struct array *ret;
    ULONG buflen, i = 0, count = 0;
    WCHAR **ptr, buf[54]; /* max IPv6 address length */

    if (!list) return NULL;
    for (gateway = list; gateway; gateway = gateway->Next) count++;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;
    if (!(ptr = heap_alloc( sizeof(*ptr) * count )))
    {
        heap_free( ret );
        return NULL;
    }
    for (gateway = list; gateway; gateway = gateway->Next)
    {
        buflen = ARRAY_SIZE( buf );
        if (WSAAddressToStringW( gateway->Address.lpSockaddr, gateway->Address.iSockaddrLength,
                                 NULL, buf, &buflen) || !(ptr[i++] = heap_strdupW( buf )))
        {
            for (; i > 0; i--) heap_free( ptr[i - 1] );
            heap_free( ptr );
            heap_free( ret );
            return NULL;
        }
    }
    ret->elem_size = sizeof(*ptr);
    ret->count     = count;
    ret->ptr       = ptr;
    return ret;
}
static struct array *get_dnsserversearchorder( IP_ADAPTER_DNS_SERVER_ADDRESS *list )
{
    IP_ADAPTER_DNS_SERVER_ADDRESS *server;
    struct array *ret;
    ULONG buflen, i = 0, count = 0;
    WCHAR **ptr, *p, buf[54]; /* max IPv6 address length */

    if (!list) return NULL;
    for (server = list; server; server = server->Next) count++;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;
    if (!(ptr = heap_alloc( sizeof(*ptr) * count )))
    {
        heap_free( ret );
        return NULL;
    }
    for (server = list; server; server = server->Next)
    {
        buflen = ARRAY_SIZE( buf );
        if (WSAAddressToStringW( server->Address.lpSockaddr, server->Address.iSockaddrLength,
                                 NULL, buf, &buflen) || !(ptr[i++] = heap_strdupW( buf )))
        {
            for (; i > 0; i--) heap_free( ptr[i - 1] );
            heap_free( ptr );
            heap_free( ret );
            return NULL;
        }
        if ((p = wcsrchr( ptr[i - 1], ':' ))) *p = 0;
    }
    ret->elem_size = sizeof(*ptr);
    ret->count     = count;
    ret->ptr       = ptr;
    return ret;
}

#ifndef __REACTOS__

static struct array *get_ipaddress( IP_ADAPTER_UNICAST_ADDRESS_LH *list )
{
    IP_ADAPTER_UNICAST_ADDRESS_LH *address;
    struct array *ret;
    ULONG buflen, i = 0, count = 0;
    WCHAR **ptr, buf[54]; /* max IPv6 address length */

    if (!list) return NULL;
    for (address = list; address; address = address->Next) count++;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;
    if (!(ptr = heap_alloc( sizeof(*ptr) * count )))
    {
        heap_free( ret );
        return NULL;
    }
    for (address = list; address; address = address->Next)
    {
        buflen = ARRAY_SIZE( buf );
        if (WSAAddressToStringW( address->Address.lpSockaddr, address->Address.iSockaddrLength,
                                 NULL, buf, &buflen) || !(ptr[i++] = heap_strdupW( buf )))
        {
            for (; i > 0; i--) heap_free( ptr[i - 1] );
            heap_free( ptr );
            heap_free( ret );
            return NULL;
        }
    }
    ret->elem_size = sizeof(*ptr);
    ret->count     = count;
    ret->ptr       = ptr;
    return ret;
}
static struct array *get_ipsubnet( IP_ADAPTER_UNICAST_ADDRESS_LH *list )
{
    IP_ADAPTER_UNICAST_ADDRESS_LH *address;
    struct array *ret;
    ULONG i = 0, count = 0;
    WCHAR **ptr;

    if (!list) return NULL;
    for (address = list; address; address = address->Next) count++;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;
    if (!(ptr = heap_alloc( sizeof(*ptr) * count )))
    {
        heap_free( ret );
        return NULL;
    }
    for (address = list; address; address = address->Next)
    {
        if (address->Address.lpSockaddr->sa_family == AF_INET)
        {
            WCHAR buf[INET_ADDRSTRLEN];
            SOCKADDR_IN addr;
            ULONG buflen = ARRAY_SIZE( buf );

            memset( &addr, 0, sizeof(addr) );
            addr.sin_family = AF_INET;
            if (ConvertLengthToIpv4Mask( address->OnLinkPrefixLength, &addr.sin_addr.S_un.S_addr ) != NO_ERROR
                    || WSAAddressToStringW( (SOCKADDR*)&addr, sizeof(addr), NULL, buf, &buflen))
                ptr[i] = NULL;
            else
                ptr[i] = heap_strdupW( buf );
        }
        else
        {
            static const WCHAR fmtW[] = {'%','u',0};
            WCHAR buf[11];

            swprintf( buf, fmtW, address->OnLinkPrefixLength );
            ptr[i] = heap_strdupW( buf );
        }
        if (!ptr[i++])
        {
            for (; i > 0; i--) heap_free( ptr[i - 1] );
            heap_free( ptr );
            heap_free( ret );
            return NULL;
        }
    }
    ret->elem_size = sizeof(*ptr);
    ret->count     = count;
    ret->ptr       = ptr;
    return ret;
}

#endif /* !__REACTOS__ */

static WCHAR *get_settingid( UINT32 index )
{
    GUID guid;
    WCHAR *ret, *str;
    memset( &guid, 0, sizeof(guid) );
    guid.Data1 = index;
    UuidToStringW( &guid, &str );
    ret = heap_strdupW( str );
    RpcStringFreeW( &str );
    return ret;
}

static enum fill_status fill_networkadapterconfig( struct table *table, const struct expr *cond )
{
    struct record_networkadapterconfig *rec;
    IP_ADAPTER_ADDRESSES *aa, *buffer;
    UINT row = 0, offset = 0, count = 0;
    DWORD size = 0, ret;
    enum fill_status status = FILL_STATUS_UNFILTERED;

    ret = GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_GATEWAYS, NULL, NULL, &size );
    if (ret != ERROR_BUFFER_OVERFLOW) return FILL_STATUS_FAILED;

    if (!(buffer = heap_alloc( size ))) return FILL_STATUS_FAILED;
    if (GetAdaptersAddresses( AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_GATEWAYS, NULL, buffer, &size ))
    {
        heap_free( buffer );
        return FILL_STATUS_FAILED;
    }
    for (aa = buffer; aa; aa = aa->Next)
    {
        if (aa->IfType != IF_TYPE_SOFTWARE_LOOPBACK) count++;
    }
    if (!resize_table( table, count, sizeof(*rec) ))
    {
        heap_free( buffer );
        return FILL_STATUS_FAILED;
    }
    for (aa = buffer; aa; aa = aa->Next)
    {
        if (aa->IfType == IF_TYPE_SOFTWARE_LOOPBACK) continue;

        rec = (struct record_networkadapterconfig *)(table->data + offset);
        rec->defaultipgateway     = get_defaultipgateway( aa->FirstGatewayAddress );
        rec->description          = heap_strdupW( aa->Description );
        rec->dhcpenabled          = -1;
        rec->dnshostname          = get_dnshostname( aa->FirstUnicastAddress );
        rec->dnsserversearchorder = get_dnsserversearchorder( aa->FirstDnsServerAddress );
        rec->index                = aa->u.s.IfIndex;
#ifndef __REACTOS__
        rec->ipaddress            = get_ipaddress( aa->FirstUnicastAddress );
#endif
        rec->ipconnectionmetric   = 20;
        rec->ipenabled            = -1;
#ifndef __REACTOS__
        rec->ipsubnet             = get_ipsubnet( aa->FirstUnicastAddress );
#endif
        rec->mac_address          = get_mac_address( aa->PhysicalAddress, aa->PhysicalAddressLength );
        rec->settingid            = get_settingid( rec->index );
        if (!match_row( table, row, cond, &status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    }
    TRACE("created %u rows\n", row);
    table->num_rows = row;

    heap_free( buffer );
    return status;
}

static enum fill_status fill_physicalmemory( struct table *table, const struct expr *cond )
{
    static const WCHAR dimm0W[] = {'D','I','M','M',' ','0',0};
    struct record_physicalmemory *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    rec = (struct record_physicalmemory *)table->data;
    rec->capacity             = get_total_physical_memory();
    rec->configuredclockspeed = 0;
    rec->devicelocator        = dimm0W;
    rec->memorytype           = 9; /* RAM */
    rec->partnumber           = NULL;
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static enum fill_status fill_pnpentity( struct table *table, const struct expr *cond )
{
    struct record_pnpentity *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    HDEVINFO device_info_set;
    SP_DEVINFO_DATA devinfo = {0};
    DWORD idx;

    device_info_set = SetupDiGetClassDevsW( NULL, NULL, NULL, DIGCF_ALLCLASSES|DIGCF_PRESENT );

    devinfo.cbSize = sizeof(devinfo);

    idx = 0;
    while (SetupDiEnumDeviceInfo( device_info_set, idx++, &devinfo ))
    {
        /* noop */
    }

    resize_table( table, idx, sizeof(*rec) );
    table->num_rows = 0;
    rec = (struct record_pnpentity *)table->data;

    idx = 0;
    while (SetupDiEnumDeviceInfo( device_info_set, idx++, &devinfo ))
    {
        WCHAR device_id[MAX_PATH];
        if (SetupDiGetDeviceInstanceIdW( device_info_set, &devinfo, device_id,
                    ARRAY_SIZE(device_id), NULL ))
        {
            rec->device_id = heap_strdupW( device_id );

            table->num_rows++;
            if (!match_row( table, table->num_rows - 1, cond, &status ))
            {
                free_row_values( table, table->num_rows - 1 );
                table->num_rows--;
            }
            else
                rec++;
        }
    }

    SetupDiDestroyDeviceInfoList( device_info_set );

    return status;
}

static enum fill_status fill_printer( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'P','r','i','n','t','e','r','%','d',0};
    struct record_printer *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    PRINTER_INFO_2W *info;
    DWORD i, offset = 0, count = 0, size = 0, num_rows = 0;
    WCHAR id[20];

    EnumPrintersW( PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &size, &count );
    if (!count) return FILL_STATUS_UNFILTERED;

    if (!(info = heap_alloc( size ))) return FILL_STATUS_FAILED;
    if (!EnumPrintersW( PRINTER_ENUM_LOCAL, NULL, 2, (BYTE *)info, size, &size, &count ))
    {
        heap_free( info );
        return FILL_STATUS_FAILED;
    }
    if (!resize_table( table, count, sizeof(*rec) ))
    {
        heap_free( info );
        return FILL_STATUS_FAILED;
    }

    for (i = 0; i < count; i++)
    {
        rec = (struct record_printer *)(table->data + offset);
        rec->attributes           = info[i].Attributes;
        swprintf( id, fmtW, i );
        rec->device_id            = heap_strdupW( id );
        rec->drivername           = heap_strdupW( info[i].pDriverName );
        rec->horizontalresolution = info[i].pDevMode->u1.s1.dmPrintQuality;
        rec->local                = -1;
        rec->location             = heap_strdupW( info[i].pLocation );
        rec->name                 = heap_strdupW( info[i].pPrinterName );
        rec->network              = 0;
        rec->portname             = heap_strdupW( info[i].pPortName );
        if (!match_row( table, i, cond, &status ))
        {
            free_row_values( table, i );
            continue;
        }
        offset += sizeof(*rec);
        num_rows++;
    }
    TRACE("created %u rows\n", num_rows);
    table->num_rows = num_rows;

    heap_free( info );
    return status;
}

static WCHAR *get_cmdline( DWORD process_id )
{
    if (process_id == GetCurrentProcessId()) return heap_strdupW( GetCommandLineW() );
    return NULL; /* FIXME handle different process case */
}

static enum fill_status fill_process( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'%','u',0};
    WCHAR handle[11];
    struct record_process *rec;
    PROCESSENTRY32W entry;
    HANDLE snap;
    enum fill_status status = FILL_STATUS_FAILED;
    UINT row = 0, offset = 0;

    snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if (snap == INVALID_HANDLE_VALUE) return FILL_STATUS_FAILED;

    entry.dwSize = sizeof(entry);
    if (!Process32FirstW( snap, &entry )) goto done;
    if (!resize_table( table, 8, sizeof(*rec) )) goto done;

    do
    {
        if (!resize_table( table, row + 1, sizeof(*rec) ))
        {
            status = FILL_STATUS_FAILED;
            goto done;
        }

        rec = (struct record_process *)(table->data + offset);
        rec->caption        = heap_strdupW( entry.szExeFile );
        rec->commandline    = get_cmdline( entry.th32ProcessID );
        rec->description    = heap_strdupW( entry.szExeFile );
        swprintf( handle, fmtW, entry.th32ProcessID );
        rec->handle         = heap_strdupW( handle );
        rec->name           = heap_strdupW( entry.szExeFile );
        rec->process_id     = entry.th32ProcessID;
        rec->pprocess_id    = entry.th32ParentProcessID;
        rec->thread_count   = entry.cntThreads;
        rec->workingsetsize = 0;
        rec->get_owner      = process_get_owner;
        if (!match_row( table, row, cond, &status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    } while (Process32NextW( snap, &entry ));

    TRACE("created %u rows\n", row);
    table->num_rows = row;

done:
    CloseHandle( snap );
    return status;
}

extern void do_cpuid( unsigned int ax, unsigned int *p );
#if defined(_MSC_VER)
void do_cpuid( unsigned int ax, unsigned int *p )
{
    __cpuid( p, ax );
}
#elif defined(__i386__)
__ASM_GLOBAL_FUNC( do_cpuid,
                   "pushl %esi\n\t"
                   "pushl %ebx\n\t"
                   "movl 12(%esp),%eax\n\t"
                   "movl 16(%esp),%esi\n\t"
                   "cpuid\n\t"
                   "movl %eax,(%esi)\n\t"
                   "movl %ebx,4(%esi)\n\t"
                   "movl %ecx,8(%esi)\n\t"
                   "movl %edx,12(%esi)\n\t"
                   "popl %ebx\n\t"
                   "popl %esi\n\t"
                   "ret" )
#elif defined(__x86_64__)
__ASM_GLOBAL_FUNC( do_cpuid,
                   "pushq %rsi\n\t"
                   "pushq %rbx\n\t"
                   "movq %rcx,%rax\n\t"
                   "movq %rdx,%rsi\n\t"
                   "cpuid\n\t"
                   "movl %eax,(%rsi)\n\t"
                   "movl %ebx,4(%rsi)\n\t"
                   "movl %ecx,8(%rsi)\n\t"
                   "movl %edx,12(%rsi)\n\t"
                   "popq %rbx\n\t"
                   "popq %rsi\n\t"
                   "ret" )
#else
void do_cpuid( unsigned int ax, unsigned int *p )
{
    FIXME("\n");
}
#endif

static unsigned int get_processor_model( unsigned int reg0, unsigned int *stepping, unsigned int *family )
{
    unsigned int model, family_id = (reg0 & (0x0f << 8)) >> 8;

    model = (reg0 & (0x0f << 4)) >> 4;
    if (family_id == 6 || family_id == 15) model |= (reg0 & (0x0f << 16)) >> 12;
    if (family)
    {
        *family = family_id;
        if (family_id == 15) *family += (reg0 & (0xff << 20)) >> 20;
    }
    *stepping = reg0 & 0x0f;
    return model;
}
static void regs_to_str( unsigned int *regs, unsigned int len, WCHAR *buffer )
{
    unsigned int i;
    unsigned char *p = (unsigned char *)regs;

    for (i = 0; i < len; i++) { buffer[i] = *p++; }
    buffer[i] = 0;
}
static void get_processor_manufacturer( WCHAR *manufacturer, UINT len )
{
    unsigned int tmp, regs[4] = {0, 0, 0, 0};

    do_cpuid( 0, regs );
    tmp = regs[2];      /* swap edx and ecx */
    regs[2] = regs[3];
    regs[3] = tmp;

    regs_to_str( regs + 1, min( 12, len ), manufacturer );
}
static const WCHAR *get_osarchitecture(void)
{
    SYSTEM_INFO info;
    GetNativeSystemInfo( &info );
    if (info.u.s.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) return os_64bitW;
    return os_32bitW;
}
static void get_processor_caption( WCHAR *caption, UINT len )
{
    static const WCHAR fmtW[] =
        {'%','s',' ','F','a','m','i','l','y',' ','%','u',' ',
         'M','o','d','e','l',' ','%','u',' ','S','t','e','p','p','i','n','g',' ','%','u',0};
    static const WCHAR x86W[] = {'x','8','6',0};
    static const WCHAR intel64W[] = {'I','n','t','e','l','6','4',0};
    static const WCHAR amd64W[] = {'A','M','D','6','4',0};
    static const WCHAR authenticamdW[] = {'A','u','t','h','e','n','t','i','c','A','M','D',0};
    const WCHAR *arch;
    WCHAR manufacturer[13];
    unsigned int regs[4] = {0, 0, 0, 0}, family, model, stepping;

    get_processor_manufacturer( manufacturer, ARRAY_SIZE( manufacturer ) );
    if (get_osarchitecture() == os_32bitW) arch = x86W;
    else if (!wcscmp( manufacturer, authenticamdW )) arch = amd64W;
    else arch = intel64W;

    do_cpuid( 1, regs );

    model = get_processor_model( regs[0], &stepping, &family );
    swprintf( caption, fmtW, arch, family, model, stepping );
}
static void get_processor_version( WCHAR *version, UINT len )
{
    static const WCHAR fmtW[] =
        {'M','o','d','e','l',' ','%','u',',',' ','S','t','e','p','p','i','n','g',' ','%','u',0};
    unsigned int regs[4] = {0, 0, 0, 0}, model, stepping;

    do_cpuid( 1, regs );

    model = get_processor_model( regs[0], &stepping, NULL );
    swprintf( version, fmtW, model, stepping );
}
static UINT16 get_processor_revision(void)
{
    unsigned int regs[4] = {0, 0, 0, 0};
    do_cpuid( 1, regs );
    return regs[0];
}
static void get_processor_id( WCHAR *processor_id, UINT len )
{
    static const WCHAR fmtW[] = {'%','0','8','X','%','0','8','X',0};
    unsigned int regs[4] = {0, 0, 0, 0};

    do_cpuid( 1, regs );
    swprintf( processor_id, fmtW, regs[3], regs[0] );
}
static void get_processor_name( WCHAR *name )
{
    unsigned int regs[4] = {0, 0, 0, 0};
    int i;

    do_cpuid( 0x80000000, regs );
    if (regs[0] >= 0x80000004)
    {
        do_cpuid( 0x80000002, regs );
        regs_to_str( regs, 16, name );
        do_cpuid( 0x80000003, regs );
        regs_to_str( regs, 16, name + 16 );
        do_cpuid( 0x80000004, regs );
        regs_to_str( regs, 16, name + 32 );
    }
    for (i = lstrlenW(name) - 1; i >= 0 && name[i] == ' '; i--) name[i] = 0;
}
static UINT get_processor_currentclockspeed( UINT index )
{
    PROCESSOR_POWER_INFORMATION *info;
    UINT ret = 1000, size = get_processor_count() * sizeof(PROCESSOR_POWER_INFORMATION);
    NTSTATUS status;

    if ((info = heap_alloc( size )))
    {
        status = NtPowerInformation( ProcessorInformation, NULL, 0, info, size );
        if (!status) ret = info[index].CurrentMhz;
        heap_free( info );
    }
    return ret;
}
static UINT get_processor_maxclockspeed( UINT index )
{
    PROCESSOR_POWER_INFORMATION *info;
    UINT ret = 1000, size = get_processor_count() * sizeof(PROCESSOR_POWER_INFORMATION);
    NTSTATUS status;

    if ((info = heap_alloc( size )))
    {
        status = NtPowerInformation( ProcessorInformation, NULL, 0, info, size );
        if (!status) ret = info[index].MaxMhz;
        heap_free( info );
    }
    return ret;
}

static enum fill_status fill_processor( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'C','P','U','%','u',0};
    WCHAR caption[100], device_id[14], processor_id[17], manufacturer[13], name[49] = {0}, version[50];
    struct record_processor *rec;
#ifdef __REACTOS__
    UINT i, offset = 0, num_rows = 0, num_cores, num_logical_processors, count = get_processor_count();
#else
    UINT i, offset = 0, num_rows = 0, num_logical, num_physical, num_packages;
#endif
    enum fill_status status = FILL_STATUS_UNFILTERED;

#ifdef __REACTOS__
    if (!resize_table( table, count, sizeof(*rec) )) return FILL_STATUS_FAILED;
#else
    num_logical = get_logical_processor_count( &num_physical, &num_packages );

    if (!resize_table( table, num_packages, sizeof(*rec) )) return FILL_STATUS_FAILED;
#endif

    get_processor_caption( caption, ARRAY_SIZE( caption ) );
    get_processor_id( processor_id, ARRAY_SIZE( processor_id ) );
    get_processor_manufacturer( manufacturer, ARRAY_SIZE( manufacturer ) );
    get_processor_name( name );
    get_processor_version( version, ARRAY_SIZE( version ) );

#ifdef __REACTOS__
    num_logical_processors = get_logical_processor_count( &num_cores ) / count;
    num_cores /= count;

    for (i = 0; i < count; i++)
#else
    for (i = 0; i < num_packages; i++)
#endif
    {
        rec = (struct record_processor *)(table->data + offset);
        rec->addresswidth           = get_osarchitecture() == os_32bitW ? 32 : 64;
        rec->architecture           = get_osarchitecture() == os_32bitW ? 0 : 9;
        rec->caption                = heap_strdupW( caption );
        rec->cpu_status             = 1; /* CPU Enabled */
        rec->currentclockspeed      = get_processor_currentclockspeed( i );
        rec->datawidth              = get_osarchitecture() == os_32bitW ? 32 : 64;
        rec->description            = heap_strdupW( caption );
        swprintf( device_id, fmtW, i );
        rec->device_id              = heap_strdupW( device_id );
        rec->family                 = 2; /* Unknown */
        rec->level                  = 15;
        rec->manufacturer           = heap_strdupW( manufacturer );
        rec->maxclockspeed          = get_processor_maxclockspeed( i );
        rec->name                   = heap_strdupW( name );
#ifdef __REACTOS__
        rec->num_cores              = num_cores;
        rec->num_logical_processors = num_logical_processors;
#else
        rec->num_cores              = num_physical / num_packages;
        rec->num_logical_processors = num_logical / num_packages;
#endif
        rec->processor_id           = heap_strdupW( processor_id );
        rec->processortype          = 3; /* central processor */
        rec->revision               = get_processor_revision();
        rec->unique_id              = NULL;
        rec->version                = heap_strdupW( version );
        if (!match_row( table, i, cond, &status ))
        {
            free_row_values( table, i );
            continue;
        }
        offset += sizeof(*rec);
        num_rows++;
    }

    TRACE("created %u rows\n", num_rows);
    table->num_rows = num_rows;
    return status;
}

static WCHAR *get_lastbootuptime(void)
{
    static const WCHAR fmtW[] =
        {'%','0','4','u','%','0','2','u','%','0','2','u','%','0','2','u','%','0','2','u','%','0','2','u',
         '.','%','0','6','u','+','0','0','0',0};
    SYSTEM_TIMEOFDAY_INFORMATION ti;
    TIME_FIELDS tf;
    WCHAR *ret;

    if (!(ret = heap_alloc( 26 * sizeof(WCHAR) ))) return NULL;

    NtQuerySystemInformation( SystemTimeOfDayInformation, &ti, sizeof(ti), NULL );
    RtlTimeToTimeFields( &ti.liKeBootTime, &tf );
    swprintf( ret, fmtW, tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds * 1000 );
    return ret;
}
static WCHAR *get_localdatetime(void)
{
    static const WCHAR fmtW[] =
        {'%','0','4','u','%','0','2','u','%','0','2','u','%','0','2','u','%','0','2','u','%','0','2','u',
         '.','%','0','6','u','%','+','0','3','d',0};
    TIME_ZONE_INFORMATION tzi;
    SYSTEMTIME st;
    WCHAR *ret;
    DWORD Status;
    LONG Bias;

    Status = GetTimeZoneInformation(&tzi);

    if(Status == TIME_ZONE_ID_INVALID) return NULL;
    Bias = tzi.Bias;
    if(Status == TIME_ZONE_ID_DAYLIGHT)
        Bias+= tzi.DaylightBias;
    else
        Bias+= tzi.StandardBias;
    if (!(ret = heap_alloc( 26 * sizeof(WCHAR) ))) return NULL;

    GetLocalTime(&st);
    swprintf( ret, fmtW, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds * 1000, -Bias );
    return ret;
}
static WCHAR *get_systemdirectory(void)
{
    void *redir;
    WCHAR *ret;

    if (!(ret = heap_alloc( MAX_PATH * sizeof(WCHAR) ))) return NULL;
    Wow64DisableWow64FsRedirection( &redir );
    GetSystemDirectoryW( ret, MAX_PATH );
    Wow64RevertWow64FsRedirection( redir );
    return ret;
}
static WCHAR *get_systemdrive(void)
{
    WCHAR *ret = heap_alloc( 3 * sizeof(WCHAR) ); /* "c:" */
    if (ret && GetEnvironmentVariableW( prop_systemdriveW, ret, 3 )) return ret;
    heap_free( ret );
    return NULL;
}
static WCHAR *get_codeset(void)
{
    static const WCHAR fmtW[] = {'%','u',0};
    WCHAR *ret = heap_alloc( 11 * sizeof(WCHAR) );
    if (ret) swprintf( ret, fmtW, GetACP() );
    return ret;
}
static WCHAR *get_countrycode(void)
{
    WCHAR *ret = heap_alloc( 6 * sizeof(WCHAR) );
    if (ret) GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, ret, 6 );
    return ret;
}
static WCHAR *get_locale(void)
{
    WCHAR *ret = heap_alloc( 5 * sizeof(WCHAR) );
    if (ret) GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, ret, 5 );
    return ret;
}
static WCHAR *get_osbuildnumber( OSVERSIONINFOEXW *ver )
{
    static const WCHAR fmtW[] = {'%','u',0};
    WCHAR *ret = heap_alloc( 11 * sizeof(WCHAR) );
    if (ret) swprintf( ret, fmtW, ver->dwBuildNumber );
    return ret;
}
static WCHAR *get_oscaption( OSVERSIONINFOEXW *ver )
{
    static const WCHAR windowsW[] =
        {'M','i','c','r','o','s','o','f','t',' ','W','i','n','d','o','w','s',' '};
    static const WCHAR win2000W[] =
        {'2','0','0','0',' ','P','r','o','f','e','s','s','i','o','n','a','l',0};
    static const WCHAR win2003W[] =
        {'S','e','r','v','e','r',' ','2','0','0','3',' ','S','t','a','n','d','a','r','d',' ','E','d','i','t','i','o','n',0};
    static const WCHAR winxpW[] =
        {'X','P',' ','P','r','o','f','e','s','s','i','o','n','a','l',0};
    static const WCHAR winxp64W[] =
        {'X','P',' ','P','r','o','f','e','s','s','i','o','n','a','l',' ','x','6','4',' ','E','d','i','t','i','o','n',0};
    static const WCHAR vistaW[] =
        {'V','i','s','t','a',' ','U','l','t','i','m','a','t','e',0};
    static const WCHAR win2008W[] =
        {'S','e','r','v','e','r',' ','2','0','0','8',' ','S','t','a','n','d','a','r','d',0};
    static const WCHAR win7W[] =
        {'7',' ','P','r','o','f','e','s','s','i','o','n','a','l',0};
    static const WCHAR win2008r2W[] =
        {'S','e','r','v','e','r',' ','2','0','0','8',' ','R','2',' ','S','t','a','n','d','a','r','d',0};
    static const WCHAR win8W[] =
        {'8',' ','P','r','o',0};
    static const WCHAR win81W[] =
        {'8','.','1',' ','P','r','o',0};
    static const WCHAR win10W[] =
        {'1','0',' ','P','r','o',0};
    int len = ARRAY_SIZE( windowsW );
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) + sizeof(win2003W) ))) return NULL;
    memcpy( ret, windowsW, sizeof(windowsW) );
    if (ver->dwMajorVersion == 10 && ver->dwMinorVersion == 0) memcpy( ret + len, win10W, sizeof(win10W) );
    else if (ver->dwMajorVersion == 6 && ver->dwMinorVersion == 3) memcpy( ret + len, win8W, sizeof(win8W) );
    else if (ver->dwMajorVersion == 6 && ver->dwMinorVersion == 2) memcpy( ret + len, win81W, sizeof(win81W) );
    else if (ver->dwMajorVersion == 6 && ver->dwMinorVersion == 1)
    {
        if (ver->wProductType == VER_NT_WORKSTATION) memcpy( ret + len, win7W, sizeof(win7W) );
        else memcpy( ret + len, win2008r2W, sizeof(win2008r2W) );
    }
    else if (ver->dwMajorVersion == 6 && ver->dwMinorVersion == 0)
    {
        if (ver->wProductType == VER_NT_WORKSTATION) memcpy( ret + len, vistaW, sizeof(vistaW) );
        else memcpy( ret + len, win2008W, sizeof(win2008W) );
    }
    else if (ver->dwMajorVersion == 5 && ver->dwMinorVersion == 2)
    {
        if (ver->wProductType == VER_NT_WORKSTATION) memcpy( ret + len, winxp64W, sizeof(winxp64W) );
        else memcpy( ret + len, win2003W, sizeof(win2003W) );
    }
    else if (ver->dwMajorVersion == 5 && ver->dwMinorVersion == 1) memcpy( ret + len, winxpW, sizeof(winxpW) );
    else memcpy( ret + len, win2000W, sizeof(win2000W) );
    return ret;
}
static WCHAR *get_osname( const WCHAR *caption )
{
    static const WCHAR partitionW[] =
        {'|','C',':','\\','W','I','N','D','O','W','S','|','\\','D','e','v','i','c','e','\\',
         'H','a','r','d','d','i','s','k','0','\\','P','a','r','t','i','t','i','o','n','1',0};
    int len = lstrlenW( caption );
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) + sizeof(partitionW) ))) return NULL;
    memcpy( ret, caption, len * sizeof(WCHAR) );
    memcpy( ret + len, partitionW, sizeof(partitionW) );
    return ret;
}
static WCHAR *get_osversion( OSVERSIONINFOEXW *ver )
{
    static const WCHAR fmtW[] = {'%','u','.','%','u','.','%','u',0};
    WCHAR *ret = heap_alloc( 33 * sizeof(WCHAR) );
    if (ret) swprintf( ret, fmtW, ver->dwMajorVersion, ver->dwMinorVersion, ver->dwBuildNumber );
    return ret;
}
#ifndef __REACTOS__
static DWORD get_operatingsystemsku(void)
{
    DWORD ret = PRODUCT_UNDEFINED;
    GetProductInfo( 6, 0, 0, 0, &ret );
    return ret;
}
#endif
static INT16 get_currenttimezone(void)
{
    TIME_ZONE_INFORMATION info;
    DWORD status = GetTimeZoneInformation( &info );
    if (status == TIME_ZONE_ID_INVALID) return 0;
    if (status == TIME_ZONE_ID_DAYLIGHT) return -(info.Bias + info.DaylightBias);
    return -(info.Bias + info.StandardBias);
}

static enum fill_status fill_operatingsystem( struct table *table, const struct expr *cond )
{
    static const WCHAR wineprojectW[] = {'T','h','e',' ','W','i','n','e',' ','P','r','o','j','e','c','t',0};
    struct record_operatingsystem *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    OSVERSIONINFOEXW ver;
    UINT row = 0;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    ver.dwOSVersionInfoSize = sizeof(ver);
    GetVersionExW( (OSVERSIONINFOW *)&ver );

    rec = (struct record_operatingsystem *)table->data;
    rec->buildnumber            = get_osbuildnumber( &ver );
    rec->caption                = get_oscaption( &ver );
    rec->codeset                = get_codeset();
    rec->countrycode            = get_countrycode();
    rec->csdversion             = ver.szCSDVersion[0] ? heap_strdupW( ver.szCSDVersion ) : NULL;
    rec->csname                 = get_computername();
    rec->currenttimezone        = get_currenttimezone();
    rec->freephysicalmemory     = get_available_physical_memory() / 1024;
    rec->installdate            = os_installdateW;
    rec->lastbootuptime         = get_lastbootuptime();
    rec->localdatetime          = get_localdatetime();
    rec->locale                 = get_locale();
    rec->manufacturer           = wineprojectW;
    rec->name                   = get_osname( rec->caption );
#ifndef __REACTOS__
    rec->operatingsystemsku     = get_operatingsystemsku();
#endif
    rec->osarchitecture         = get_osarchitecture();
    rec->oslanguage             = GetSystemDefaultLangID();
    rec->osproductsuite         = 2461140; /* Windows XP Professional  */
    rec->ostype                 = 18;      /* WINNT */
    rec->primary                = -1;
    rec->serialnumber           = os_serialnumberW;
    rec->servicepackmajor       = ver.wServicePackMajor;
    rec->servicepackminor       = ver.wServicePackMinor;
    rec->suitemask              = 272;     /* Single User + Terminal */
    rec->systemdirectory        = get_systemdirectory();
    rec->systemdrive            = get_systemdrive();
    rec->totalvirtualmemorysize = get_total_physical_memory() / 1024;
    rec->totalvisiblememorysize = rec->totalvirtualmemorysize;
    rec->version                = get_osversion( &ver );
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

static const WCHAR *get_service_type( DWORD type )
{
    static const WCHAR filesystem_driverW[] =
        {'F','i','l','e',' ','S','y','s','t','e','m',' ','D','r','i','v','e','r',0};
    static const WCHAR kernel_driverW[] =
        {'K','e','r','n','e','l',' ','D','r','i','v','e','r',0};
    static const WCHAR own_processW[] =
        {'O','w','n',' ','P','r','o','c','e','s','s',0};
    static const WCHAR share_processW[] =
        {'S','h','a','r','e',' ','P','r','o','c','e','s','s',0};

    if (type & SERVICE_KERNEL_DRIVER)            return kernel_driverW;
    else if (type & SERVICE_FILE_SYSTEM_DRIVER)  return filesystem_driverW;
    else if (type & SERVICE_WIN32_OWN_PROCESS)   return own_processW;
    else if (type & SERVICE_WIN32_SHARE_PROCESS) return share_processW;
    else ERR("unhandled type 0x%08x\n", type);
    return NULL;
}
static const WCHAR *get_service_state( DWORD state )
{
    static const WCHAR runningW[] =
        {'R','u','n','n','i','n','g',0};
    static const WCHAR start_pendingW[] =
        {'S','t','a','r','t',' ','P','e','n','d','i','n','g',0};
    static const WCHAR stop_pendingW[] =
        {'S','t','o','p',' ','P','e','n','d','i','n','g',0};
    static const WCHAR stoppedW[] =
        {'S','t','o','p','p','e','d',0};
    static const WCHAR unknownW[] =
        {'U','n','k','n','o','w','n',0};

    switch (state)
    {
    case SERVICE_STOPPED:       return stoppedW;
    case SERVICE_START_PENDING: return start_pendingW;
    case SERVICE_STOP_PENDING:  return stop_pendingW;
    case SERVICE_RUNNING:       return runningW;
    default:
        ERR("unknown state %u\n", state);
        return unknownW;
    }
}
static const WCHAR *get_service_startmode( DWORD mode )
{
    static const WCHAR bootW[] = {'B','o','o','t',0};
    static const WCHAR systemW[] = {'S','y','s','t','e','m',0};
    static const WCHAR autoW[] = {'A','u','t','o',0};
    static const WCHAR manualW[] = {'M','a','n','u','a','l',0};
    static const WCHAR disabledW[] = {'D','i','s','a','b','l','e','d',0};
    static const WCHAR unknownW[] = {'U','n','k','n','o','w','n',0};

    switch (mode)
    {
    case SERVICE_BOOT_START:   return bootW;
    case SERVICE_SYSTEM_START: return systemW;
    case SERVICE_AUTO_START:   return autoW;
    case SERVICE_DEMAND_START: return manualW;
    case SERVICE_DISABLED:     return disabledW;
    default:
        ERR("unknown mode 0x%x\n", mode);
        return unknownW;
    }
}
static QUERY_SERVICE_CONFIGW *query_service_config( SC_HANDLE manager, const WCHAR *name )
{
    QUERY_SERVICE_CONFIGW *config = NULL;
    SC_HANDLE service;
    DWORD size;

    if (!(service = OpenServiceW( manager, name, SERVICE_QUERY_CONFIG ))) return NULL;
    QueryServiceConfigW( service, NULL, 0, &size );
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) goto done;
    if (!(config = heap_alloc( size ))) goto done;
    if (QueryServiceConfigW( service, config, size, &size )) goto done;
    heap_free( config );
    config = NULL;

done:
    CloseServiceHandle( service );
    return config;
}

static enum fill_status fill_service( struct table *table, const struct expr *cond )
{
    struct record_service *rec;
    SC_HANDLE manager;
    ENUM_SERVICE_STATUS_PROCESSW *tmp, *services = NULL;
    SERVICE_STATUS_PROCESS *status;
    WCHAR sysnameW[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = ARRAY_SIZE( sysnameW );
    UINT i, row = 0, offset = 0, size = 256, needed, count;
    enum fill_status fill_status = FILL_STATUS_FAILED;
    BOOL ret;

    if (!(manager = OpenSCManagerW( NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE ))) return FILL_STATUS_FAILED;
    if (!(services = heap_alloc( size ))) goto done;

    ret = EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL,
                                 SERVICE_STATE_ALL, (BYTE *)services, size, &needed,
                                 &count, NULL, NULL );
    if (!ret)
    {
        if (GetLastError() != ERROR_MORE_DATA) goto done;
        size = needed;
        if (!(tmp = heap_realloc( services, size ))) goto done;
        services = tmp;
        ret = EnumServicesStatusExW( manager, SC_ENUM_PROCESS_INFO, SERVICE_TYPE_ALL,
                                     SERVICE_STATE_ALL, (BYTE *)services, size, &needed,
                                     &count, NULL, NULL );
        if (!ret) goto done;
    }
    if (!resize_table( table, count, sizeof(*rec) )) goto done;

    GetComputerNameW( sysnameW, &len );
    fill_status = FILL_STATUS_UNFILTERED;

    for (i = 0; i < count; i++)
    {
        QUERY_SERVICE_CONFIGW *config;

        if (!(config = query_service_config( manager, services[i].lpServiceName ))) continue;

        status = &services[i].ServiceStatusProcess;
        rec = (struct record_service *)(table->data + offset);
        rec->accept_pause   = (status->dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE) ? -1 : 0;
        rec->accept_stop    = (status->dwControlsAccepted & SERVICE_ACCEPT_STOP) ? -1 : 0;
        rec->displayname    = heap_strdupW( services[i].lpDisplayName );
        rec->name           = heap_strdupW( services[i].lpServiceName );
        rec->process_id     = status->dwProcessId;
        rec->servicetype    = get_service_type( status->dwServiceType );
        rec->startmode      = get_service_startmode( config->dwStartType );
        rec->state          = get_service_state( status->dwCurrentState );
        rec->systemname     = heap_strdupW( sysnameW );
        rec->pause_service  = service_pause_service;
        rec->resume_service = service_resume_service;
        rec->start_service  = service_start_service;
        rec->stop_service   = service_stop_service;
        heap_free( config );
        if (!match_row( table, row, cond, &fill_status ))
        {
            free_row_values( table, row );
            continue;
        }
        offset += sizeof(*rec);
        row++;
    }

    TRACE("created %u rows\n", row);
    table->num_rows = row;

done:
    CloseServiceHandle( manager );
    heap_free( services );
    return fill_status;
}

static WCHAR *get_accountname( LSA_TRANSLATED_NAME *name )
{
    if (!name || !name->Name.Buffer) return NULL;
    return heap_strdupW( name->Name.Buffer );
}
static struct array *get_binaryrepresentation( PSID sid, UINT len )
{
    struct array *ret;
    UINT8 *ptr;

    if (!(ret = heap_alloc( sizeof(*ret) ))) return NULL;
    if (!(ptr = heap_alloc( len )))
    {
        heap_free( ret );
        return NULL;
    }
    memcpy( ptr, sid, len );
    ret->elem_size = sizeof(*ptr);
    ret->count     = len;
    ret->ptr       = ptr;
    return ret;
}
static WCHAR *get_referenceddomainname( LSA_REFERENCED_DOMAIN_LIST *domain )
{
    if (!domain || !domain->Domains || !domain->Domains->Name.Buffer) return NULL;
    return heap_strdupW( domain->Domains->Name.Buffer );
}
static const WCHAR *find_sid_str( const struct expr *cond )
{
    const struct expr *left, *right;
    const WCHAR *ret = NULL;

    if (!cond || cond->type != EXPR_COMPLEX || cond->u.expr.op != OP_EQ) return NULL;

    left = cond->u.expr.left;
    right = cond->u.expr.right;
    if (left->type == EXPR_PROPVAL && right->type == EXPR_SVAL && !wcsicmp( left->u.propval->name, prop_sidW ))
    {
        ret = right->u.sval;
    }
    else if (left->type == EXPR_SVAL && right->type == EXPR_PROPVAL && !wcsicmp( right->u.propval->name, prop_sidW ))
    {
        ret = left->u.sval;
    }
    return ret;
}

static enum fill_status fill_sid( struct table *table, const struct expr *cond )
{
    PSID sid;
    LSA_REFERENCED_DOMAIN_LIST *domain;
    LSA_TRANSLATED_NAME *name;
    LSA_HANDLE handle;
    LSA_OBJECT_ATTRIBUTES attrs;
    const WCHAR *str;
    struct record_sid *rec;
    UINT len;

    if (!(str = find_sid_str( cond ))) return FILL_STATUS_FAILED;
    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    if (!ConvertStringSidToSidW( str, &sid )) return FILL_STATUS_FAILED;
    len = GetLengthSid( sid );

    memset( &attrs, 0, sizeof(attrs) );
    attrs.Length = sizeof(attrs);
    if (LsaOpenPolicy( NULL, &attrs, POLICY_ALL_ACCESS, &handle ))
    {
        LocalFree( sid );
        return FILL_STATUS_FAILED;
    }
    if (LsaLookupSids( handle, 1, &sid, &domain, &name ))
    {
        LocalFree( sid );
        LsaClose( handle );
        return FILL_STATUS_FAILED;
    }

    rec = (struct record_sid *)table->data;
    rec->accountname            = get_accountname( name );
    rec->binaryrepresentation   = get_binaryrepresentation( sid, len );
    rec->referenceddomainname   = get_referenceddomainname( domain );
    rec->sid                    = heap_strdupW( str );
    rec->sidlength              = len;

    TRACE("created 1 row\n");
    table->num_rows = 1;

    LsaFreeMemory( domain );
    LsaFreeMemory( name );
    LocalFree( sid );
    LsaClose( handle );
    return FILL_STATUS_FILTERED;
}

static WCHAR *get_systemenclosure_string( BYTE id, const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_chassis *chassis;
    UINT offset;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_CHASSIS, buf, len ))) return NULL;

    chassis = (const struct smbios_chassis *)hdr;
    offset = (const char *)chassis - buf + chassis->hdr.length;
    return get_smbios_string( id, buf, offset, len );
}

static WCHAR *get_systemenclosure_manufacturer( const char *buf, UINT len )
{
    WCHAR *ret = get_systemenclosure_string( 1, buf, len );
    if (!ret) return heap_strdupW( systemenclosure_manufacturerW );
    return ret;
}

static int get_systemenclosure_lockpresent( const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_chassis *chassis;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_CHASSIS, buf, len )) || hdr->length < sizeof(*chassis)) return 0;

    chassis = (const struct smbios_chassis *)hdr;
    return (chassis->type & 0x80) ? -1 : 0;
}

static struct array *dup_array( const struct array *src )
{
    struct array *dst;
    if (!(dst = heap_alloc( sizeof(*dst) ))) return NULL;
    if (!(dst->ptr = heap_alloc( src->count * src->elem_size )))
    {
        heap_free( dst );
        return NULL;
    }
    memcpy( dst->ptr, src->ptr, src->count * src->elem_size );
    dst->elem_size = src->elem_size;
    dst->count     = src->count;
    return dst;
}

static struct array *get_systemenclosure_chassistypes( const char *buf, UINT len )
{
    const struct smbios_header *hdr;
    const struct smbios_chassis *chassis;
    struct array *ret = NULL;
    UINT16 *types;

    if (!(hdr = find_smbios_entry( SMBIOS_TYPE_CHASSIS, buf, len )) || hdr->length < sizeof(*chassis)) goto done;
    chassis = (const struct smbios_chassis *)hdr;

    if (!(ret = heap_alloc( sizeof(*ret) ))) goto done;
    if (!(types = heap_alloc( sizeof(*types) )))
    {
        heap_free( ret );
        return NULL;
    }
    types[0] = chassis->type & ~0x80;

    ret->elem_size = sizeof(*types);
    ret->count     = 1;
    ret->ptr       = types;

done:
    if (!ret) ret = dup_array( &systemenclosure_chassistypes_array );
    return ret;
}

static enum fill_status fill_systemenclosure( struct table *table, const struct expr *cond )
{
    struct record_systemenclosure *rec;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    UINT row = 0, len;
    char *buf;

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    len = GetSystemFirmwareTable( RSMB, 0, NULL, 0 );
    if (!(buf = heap_alloc( len ))) return FILL_STATUS_FAILED;
    GetSystemFirmwareTable( RSMB, 0, buf, len );

    rec = (struct record_systemenclosure *)table->data;
    rec->caption      = systemenclosure_systemenclosureW;
    rec->chassistypes = get_systemenclosure_chassistypes( buf, len );
    rec->description  = systemenclosure_systemenclosureW;
    rec->lockpresent  = get_systemenclosure_lockpresent( buf, len );
    rec->manufacturer = get_systemenclosure_manufacturer( buf, len );
    rec->name         = systemenclosure_systemenclosureW;
    rec->tag          = systemenclosure_tagW;
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    heap_free( buf );

    TRACE("created %u rows\n", row);
    table->num_rows = row;
    return status;
}

#ifndef __REACTOS__
static WCHAR *get_pnpdeviceid( DXGI_ADAPTER_DESC *desc )
{
    static const WCHAR fmtW[] =
        {'P','C','I','\\','V','E','N','_','%','0','4','X','&','D','E','V','_','%','0','4','X',
         '&','S','U','B','S','Y','S','_','%','0','8','X','&','R','E','V','_','%','0','2','X','\\',
         '0','&','D','E','A','D','B','E','E','F','&','0','&','D','E','A','D',0};
    UINT len = sizeof(fmtW) + 2;
    WCHAR *ret;

    if (!(ret = heap_alloc( len * sizeof(WCHAR) ))) return NULL;
    swprintf( ret, fmtW, desc->VendorId, desc->DeviceId, desc->SubSysId, desc->Revision );
    return ret;
}
#endif

#define HW_VENDOR_AMD    0x1002
#define HW_VENDOR_NVIDIA 0x10de
#define HW_VENDOR_VMWARE 0x15ad
#define HW_VENDOR_INTEL  0x8086

#ifndef __REACTOS__

static const WCHAR *get_installeddriver( UINT vendorid )
{
    static const WCHAR driver_amdW[] = {'a','t','i','c','f','x','3','2','.','d','l','l',0};
    static const WCHAR driver_intelW[] = {'i','g','d','u','m','d','i','m','3','2','.','d','l','l',0};
    static const WCHAR driver_nvidiaW[] = {'n','v','d','3','d','u','m','.','d','l','l',0};
    static const WCHAR driver_wineW[] = {'w','i','n','e','.','d','l','l',0};

    /* FIXME: wined3d has a better table, but we cannot access this information through dxgi */

    if (vendorid == HW_VENDOR_AMD)
        return driver_amdW;
    else if (vendorid == HW_VENDOR_NVIDIA)
        return driver_nvidiaW;
    else if (vendorid == HW_VENDOR_INTEL)
        return driver_intelW;
    return driver_wineW;
}

static enum fill_status fill_videocontroller( struct table *table, const struct expr *cond )
{
    static const WCHAR fmtW[] = {'%','u',' ','x',' ','%','u',' ','x',' ','%','I','6','4','u',' ','c','o','l','o','r','s',0};
    struct record_videocontroller *rec;
    HRESULT hr;
    IDXGIFactory *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    DXGI_ADAPTER_DESC desc;
    UINT row = 0, hres = 1024, vres = 768, vidmem = 512 * 1024 * 1024;
    const WCHAR *name = videocontroller_deviceidW;
    enum fill_status status = FILL_STATUS_UNFILTERED;
    WCHAR mode[44];

    if (!resize_table( table, 1, sizeof(*rec) )) return FILL_STATUS_FAILED;

    memset (&desc, 0, sizeof(desc));
    hr = CreateDXGIFactory( &IID_IDXGIFactory, (void **)&factory );
    if (FAILED(hr)) goto done;

    hr = IDXGIFactory_EnumAdapters( factory, 0, &adapter );
    if (FAILED(hr)) goto done;

    hr = IDXGIAdapter_GetDesc( adapter, &desc );
    if (SUCCEEDED(hr))
    {
        vidmem = desc.DedicatedVideoMemory;
        name   = desc.Description;
    }

done:
    rec = (struct record_videocontroller *)table->data;
    rec->adapter_dactype       = videocontroller_dactypeW;
    rec->adapter_ram           = vidmem;
    rec->availability          = 3; /* Running or Full Power */
    rec->config_errorcode      = 0; /* no error */
    rec->caption               = heap_strdupW( name );
    rec->current_bitsperpixel  = get_bitsperpixel( &hres, &vres );
    rec->current_horizontalres = hres;
    rec->current_refreshrate   = 0; /* default refresh rate */
    rec->current_scanmode      = 2; /* Unknown */
    rec->current_verticalres   = vres;
    rec->description           = heap_strdupW( name );
    rec->device_id             = videocontroller_deviceidW;
    rec->driverdate            = videocontroller_driverdateW;
    rec->driverversion         = videocontroller_driverversionW;
    rec->installeddriver       = get_installeddriver( desc.VendorId );
    rec->name                  = heap_strdupW( name );
    rec->pnpdevice_id          = get_pnpdeviceid( &desc );
    rec->status                = videocontroller_statusW;
    rec->videoarchitecture     = 2; /* Unknown */
    rec->videomemorytype       = 2; /* Unknown */
    swprintf( mode, fmtW, hres, vres, (UINT64)1 << rec->current_bitsperpixel );
    rec->videomodedescription  = heap_strdupW( mode );
    rec->videoprocessor        = heap_strdupW( name );
    if (!match_row( table, row, cond, &status )) free_row_values( table, row );
    else row++;

    TRACE("created %u rows\n", row);
    table->num_rows = row;

    if (adapter) IDXGIAdapter_Release( adapter );
    if (factory) IDXGIFactory_Release( factory );
    return status;
}

#endif /* !__REACTOS__ */

#define C(c) sizeof(c)/sizeof(c[0]), c
#define D(d) sizeof(d)/sizeof(d[0]), 0, (BYTE *)d
static struct table builtin_classes[] =
{
    { class_associatorsW, C(col_associator), D(data_associator) },
    { class_baseboardW, C(col_baseboard), 0, 0, NULL, fill_baseboard },
    { class_biosW, C(col_bios), 0, 0, NULL, fill_bios },
    { class_cdromdriveW, C(col_cdromdrive), 0, 0, NULL, fill_cdromdrive },
    { class_compsysW, C(col_compsys), 0, 0, NULL, fill_compsys },
    { class_compsysproductW, C(col_compsysproduct), 0, 0, NULL, fill_compsysproduct },
    { class_datafileW, C(col_datafile), 0, 0, NULL, fill_datafile },
    { class_desktopmonitorW, C(col_desktopmonitor), 0, 0, NULL, fill_desktopmonitor },
    { class_directoryW, C(col_directory), 0, 0, NULL, fill_directory },
    { class_diskdriveW, C(col_diskdrive), 0, 0, NULL, fill_diskdrive },
    { class_diskdrivetodiskpartitionW, C(col_diskdrivetodiskpartition), 0, 0, NULL, fill_diskdrivetodiskpartition },
    { class_diskpartitionW, C(col_diskpartition), 0, 0, NULL, fill_diskpartition },
    { class_displaycontrollerconfigW, C(col_displaycontrollerconfig), 0, 0, NULL, fill_displaycontrollerconfig },
    { class_ip4routetableW, C(col_ip4routetable), 0, 0, NULL, fill_ip4routetable },
    { class_logicaldiskW, C(col_logicaldisk), 0, 0, NULL, fill_logicaldisk },
    { class_logicaldisk2W, C(col_logicaldisk), 0, 0, NULL, fill_logicaldisk },
    { class_logicaldisktopartitionW, C(col_logicaldisktopartition), 0, 0, NULL, fill_logicaldisktopartition },
    { class_networkadapterW, C(col_networkadapter), 0, 0, NULL, fill_networkadapter },
    { class_networkadapterconfigW, C(col_networkadapterconfig), 0, 0, NULL, fill_networkadapterconfig },
    { class_operatingsystemW, C(col_operatingsystem), 0, 0, NULL, fill_operatingsystem },
    { class_paramsW, C(col_param), D(data_param) },
    { class_physicalmediaW, C(col_physicalmedia), D(data_physicalmedia) },
    { class_physicalmemoryW, C(col_physicalmemory), 0, 0, NULL, fill_physicalmemory },
    { class_pnpentityW, C(col_pnpentity), 0, 0, NULL, fill_pnpentity },
    { class_printerW, C(col_printer), 0, 0, NULL, fill_printer },
    { class_processW, C(col_process), 0, 0, NULL, fill_process },
    { class_processorW, C(col_processor), 0, 0, NULL, fill_processor },
    { class_processor2W, C(col_processor), 0, 0, NULL, fill_processor },
    { class_qualifiersW, C(col_qualifier), D(data_qualifier) },
    { class_quickfixengineeringW, C(col_quickfixengineering), D(data_quickfixengineering) },
    { class_serviceW, C(col_service), 0, 0, NULL, fill_service },
    { class_sidW, C(col_sid), 0, 0, NULL, fill_sid },
    { class_sounddeviceW, C(col_sounddevice), D(data_sounddevice) },
    { class_stdregprovW, C(col_stdregprov), D(data_stdregprov) },
    { class_systemsecurityW, C(col_systemsecurity), D(data_systemsecurity) },
    { class_systemenclosureW, C(col_systemenclosure), 0, 0, NULL, fill_systemenclosure },
#ifndef __REACTOS__
    /* Requires dxgi.dll */
    { class_videocontrollerW, C(col_videocontroller), 0, 0, NULL, fill_videocontroller },
#endif
    { class_winsatW, C(col_winsat), D(data_winsat) },
};
#undef C
#undef D

void init_table_list( void )
{
    static struct list tables = LIST_INIT( tables );
    UINT i;

    for (i = 0; i < ARRAY_SIZE(builtin_classes); i++) list_add_tail( &tables, &builtin_classes[i].entry );
    table_list = &tables;
}
