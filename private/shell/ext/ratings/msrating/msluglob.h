#if !defined (EXTERN)
#define EXTERN extern
#endif

#if !defined (ASSIGN)
#define ASSIGN(value)
#endif

/* the 'extern' must be forced for constant arrays, because 'const'
 * in C++ implies 'static' otherwise.
 */
#define EXTTEXT(n) extern const CHAR n[]
#define TEXTCONST(name,text) EXTTEXT(name) ASSIGN(text)

TEXTCONST(szHelpFile,"ratings.hlp");
/*  TEXTCONST(szProfileList,REGSTR_PATH_SETUP "\\ProfileList"); */
/* TEXTCONST(szSupervisor,"Supervisor"); */
TEXTCONST(szDefaultUserName,".Default");
TEXTCONST(szRatingsSupervisorKeyName,"Key");
TEXTCONST(szUsersSupervisorKeyName,"Key2");
TEXTCONST(szLogonKey,"Network\\Logon");
TEXTCONST(szUserProfiles,"UserProfiles");
TEXTCONST(szPOLICYKEY,      "System\\CurrentControlSet\\Control\\Update");
TEXTCONST(szPOLICYVALUE,    "UpdateMode");

TEXTCONST(szComDlg32,"comdlg32.dll");
TEXTCONST(szShell32,"shell32.dll");
TEXTCONST(szGetOpenFileName,"GetOpenFileNameA");    // we're ANSI, even on NT
TEXTCONST(szShellExecute,"ShellExecuteA");

TEXTCONST(VAL_UNKNOWNS,"Allow_Unknowns");
/* TEXTCONST(VAL_MASTERUSER,"MasterUser"); */
TEXTCONST(VAL_PLEASEMOM,"PleaseMom");
TEXTCONST(VAL_ENABLED,"Enabled");
#ifdef NASH
TEXTCONST(VAL_CONTROLPANEL, "ControlPanel");
TEXTCONST(VAL_NEWAPPS,      "NewApps");
TEXTCONST(szBETTERNAME,     "*Guest User*");     //BUGBUG
#endif

TEXTCONST(szPOLUSER,        "PolicyData\\Users");
TEXTCONST(szTMPDATA,        "PolicyData");
TEXTCONST(szUSERS,          "Users");
TEXTCONST(szRATINGS,        "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Ratings");
TEXTCONST(szRATINGHELPERS,  "Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Rating Helpers");
TEXTCONST(szRORSGUID,       "{20EDB660-7CDD-11CF-8DAB-00AA006C1A01}");
TEXTCONST(szCLSID,          "CLSID");
TEXTCONST(szINPROCSERVER32, "InProcServer32");
TEXTCONST(szDLLNAME,        "msrating.dll");
TEXTCONST(szTHREADINGMODEL, "ThreadingModel");
TEXTCONST(szAPARTMENT,      "Apartment");

TEXTCONST(szPOLFILE,        "ratings.pol");
TEXTCONST(szBACKSLASH,      "\\");
TEXTCONST(szDEFAULTRATFILE, "RSACi.rat");
TEXTCONST(szFilenameTemplate, "FileName%d");        /* note, mslubase.cpp knows the length of this string is 8 + number length */
TEXTCONST(szNULL,           "");
TEXTCONST(szRATINGBUREAU,   "Bureau");

/* t-markh 8/98 - Text strings used in parsing PICSRules */

TEXTCONST(szPRShortYes,"y");
TEXTCONST(szPRYes,"yes");
TEXTCONST(szPRShortNo,"n");
TEXTCONST(szPRNo,"no");
TEXTCONST(szPRPass,"pass");
TEXTCONST(szPRFail,"fail");

//t-markh, These are not in the official spec, but we should handle them anyway
TEXTCONST(szPRShortPass,"p");
TEXTCONST(szPRShortFail,"f");


/* Text strings used in parsing rating labels. */

TEXTCONST(szDoubleCRLF,"\r\n\r\n");
TEXTCONST(szPicsOpening,"(PICS-");
TEXTCONST(szWhitespace," \t\r\n");
TEXTCONST(szExtendedAlphaNum,"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+-.,;:&=?!*~@#/");
TEXTCONST(szSingleCharTokens,"()\"");
TEXTCONST(szLeftParen,"(");
TEXTCONST(szRightParen,")");
TEXTCONST(szOptional,"optional");
TEXTCONST(szMandatory,"mandatory");
TEXTCONST(szAtOption,"at");
TEXTCONST(szByOption,"by");
TEXTCONST(szCommentOption,"comment");
TEXTCONST(szCompleteLabelOption,"complete-label");
TEXTCONST(szFullOption,"full");
TEXTCONST(szExtensionOption,"extension");
TEXTCONST(szGenericOption,"generic");
TEXTCONST(szShortGenericOption,"gen");
TEXTCONST(szForOption,"for");
TEXTCONST(szMICOption,"MIC-md5");
TEXTCONST(szMD5Option,"md5");
TEXTCONST(szOnOption,"on");
TEXTCONST(szSigOption,"signature-PKCS");
TEXTCONST(szUntilOption,"until");
TEXTCONST(szExpOption,"exp");
TEXTCONST(szRatings,"ratings");
/* TEXTCONST(szShortRatings,"r"); */
TEXTCONST(szError,"error");
TEXTCONST(szNoRatings,"no-ratings");
TEXTCONST(szLabelWord,"labels");
/* TEXTCONST(szShortLabelWord,"l"); */
TEXTCONST(szShortTrue,"t");
TEXTCONST(szTrue,"true");
TEXTCONST(szShortFalse,"f");
TEXTCONST(szFalse,"false");

/* TEXTCONST(szNegInf,"-INF"); */
/* TEXTCONST(szPosInf,"+INF"); */
TEXTCONST(szLabel,"label");
TEXTCONST(szName,"name");
TEXTCONST(szValue,"value");
TEXTCONST(szIcon,"icon");
TEXTCONST(szDescription, "description");
TEXTCONST(szCategory, "category");
TEXTCONST(szTransmitAs, "transmit-as");
TEXTCONST(szMin,"min");
TEXTCONST(szMax,"max");
/* TEXTCONST(szMultivalue,"multivalue"); */
TEXTCONST(szInteger,"integer");
TEXTCONST(szLabelOnly, "label-only");
TEXTCONST(szPicsVersion,"PICS-version");
TEXTCONST(szRatingSystem,"rating-system");
TEXTCONST(szRatingService,"rating-service");
TEXTCONST(szRatingBureau,"rating-bureau");
TEXTCONST(szBureauRequired,"bureau-required");
TEXTCONST(szDefault,"default");
TEXTCONST(szMultiValue,"multivalue");
TEXTCONST(szUnordered,"unordered");
TEXTCONST(szRatingBureauExtension,"www.w3.org/PICS/service-extensions/label-bureau");

EXTERN CHAR abSupervisorKey[16] ASSIGN({0});        /* supervisor password hash */
EXTERN CHAR fSupervisorKeyInit ASSIGN(FALSE);       /* whether abSupervisorKey has been initialized */

//t-markh 8/98
//The following TEXTCONST's are for PICSRules support.
//Dereferenced in picsrule.cpp
TEXTCONST(szPICSRulesVersion,"PicsRule");
TEXTCONST(szPICSRulesPolicy,"Policy");
TEXTCONST(szPICSRulesExplanation,"Explanation");
TEXTCONST(szPICSRulesRejectByURL,"RejectByURL");
TEXTCONST(szPICSRulesAcceptByURL,"AcceptByURL");
TEXTCONST(szPICSRulesRejectIf,"RejectIf");
TEXTCONST(szPICSRulesAcceptIf,"AcceptIf");
TEXTCONST(szPICSRulesAcceptUnless,"AcceptUnless");
TEXTCONST(szPICSRulesRejectUnless,"RejectUnless");
TEXTCONST(szPICSRulesName,"name");
TEXTCONST(szPICSRulesRuleName,"Rulename");
TEXTCONST(szPICSRulesDescription,"Description");
TEXTCONST(szPICSRulesSource,"source");
TEXTCONST(szPICSRulesSourceURL,"SourceURL");
TEXTCONST(szPICSRulesCreationTool,"CreationTool");
TEXTCONST(szPICSRulesAuthor,"author");
TEXTCONST(szPICSRulesLastModified,"LastModified");
TEXTCONST(szPICSRulesServiceInfo,"serviceinfo");
TEXTCONST(szPICSRulesSIName,"Name");
TEXTCONST(szPICSRulesShortName,"shortname");
TEXTCONST(szPICSRulesBureauURL,"BureauURL");
TEXTCONST(szPICSRulesUseEmbedded,"UseEmbedded");
TEXTCONST(szPICSRulesRATFile,"Ratfile");
TEXTCONST(szPICSRulesBureauUnavailable,"BureauUnavailable");
TEXTCONST(szPICSRulesOptExtension,"optextension");
TEXTCONST(szPICSRulesExtensionName,"extension-name");
TEXTCONST(szPICSRulesReqExtension,"reqextension");
TEXTCONST(szPICSRulesExtension,"Extension");
TEXTCONST(szPICSRulesOptionDefault,"OptionDefault");
TEXTCONST(szPICSRulesDegenerateExpression,"otherwise");
TEXTCONST(szPICSRulesOr,"or");
TEXTCONST(szPICSRulesAnd,"and");
TEXTCONST(szPICSRulesHTTP,"http");
TEXTCONST(szPICSRulesFTP,"ftp");
TEXTCONST(szPICSRulesGOPHER,"gopher");
TEXTCONST(szPICSRulesNNTP,"nntp");
TEXTCONST(szPICSRulesIRC,"irc");
TEXTCONST(szPICSRulesPROSPERO,"perospero");
TEXTCONST(szPICSRulesTELNET,"telnet");
TEXTCONST(szFINDSYSTEM,"http://www.microsoft.com/isapi/redir.dll?prd=ie&ar=ratings&pver=5.0");
TEXTCONST(szTURNOFF,"WarnOnOff");

//These TEXTCONSTS are purposly obfucated to discourage those who would
//from tampering with our settings in the registry
//t-markh - BUGBUG - need to obfuscate names after debugging
TEXTCONST(szPICSRULESSYSTEMNAME,"Name");
TEXTCONST(szPICSRULESFILENAME,"FileName");
TEXTCONST(szPICSRULESSYSTEMS,"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Ratings\\PICSRules");
TEXTCONST(szPICSRULESNUMSYS,"NumSys");
TEXTCONST(szPICSRULESVERMAJOR,"MajorVer");
TEXTCONST(szPICSRULESVERMINOR,"MinorVer");
TEXTCONST(szPICSRULESDWFLAGS,"dwFlags");
TEXTCONST(szPICSRULESERRLINE,"errLine");
TEXTCONST(szPICSRULESPRNAME,"PRName");
TEXTCONST(szPICSRULESRULENAME,"RULEName");
TEXTCONST(szPICSRULESDESCRIPTION,"Description");
TEXTCONST(szPICSRULESPRSOURCE,"PRSource");
TEXTCONST(szPICSRULESSOURCEURL,"SourceURL");
TEXTCONST(szPICSRULESEXPRESSIONEMBEDDED,"PREEmbedded");
TEXTCONST(szPICSRULESEXPRESSIONSERVICENAME,"PREServiceName");
TEXTCONST(szPICSRULESEXPRESSIONCATEGORYNAME,"PRECategoryName");
TEXTCONST(szPICSRULESEXPRESSIONFULLSERVICENAME,"PREFullServiceName");
TEXTCONST(szPICSRULESEXPRESSIONVALUE,"PREValue");
TEXTCONST(szPICSRULESEXPRESSIONPOLICYOPERATOR,"PREOperator");
TEXTCONST(szPICSRULESEXPRESSIONOPPOLICYEMBEDDED,"PREPolEmbedded");
TEXTCONST(szPICSRULESEXPRESSIONLEFT,"PREEmbeddedLeft");
TEXTCONST(szPICSRULESEXPRESSIONRIGHT,"PREEmbeddedRight");
TEXTCONST(szPICSRULESCREATIONTOOL,"PRCreationTool");
TEXTCONST(szPICSRULESEMAILAUTHOR,"PREmailAuthor");
TEXTCONST(szPICSRULESLASTMODIFIED,"PRLastModified");
TEXTCONST(szPICSRULESPRPOLICY,"PRPolicy");
TEXTCONST(szPICSRULESNUMPOLICYS,"PRNumPolicy");
TEXTCONST(szPICSRULESPOLICYEXPLANATION,"PRPExplanation");
TEXTCONST(szPICSRULESPOLICYATTRIBUTE,"PRPPolicyAttribute");
TEXTCONST(szPICSRULESPOLICYSUB,"PRPPolicySub");
TEXTCONST(szPICSRULESBYURLINTERNETPATTERN,"PRBUInternetPattern");
TEXTCONST(szPICSRULESBYURLNONWILD,"PRBUNonWild");
TEXTCONST(szPICSRULESBYURLSPECIFIED,"PRBUSpecified");
TEXTCONST(szPICSRULESBYURLSCHEME,"PRBUScheme");
TEXTCONST(szPICSRULESBYURLUSER,"PRBUUser");
TEXTCONST(szPICSRULESBYURLHOST,"PRBUHost");
TEXTCONST(szPICSRULESBYURLPORT,"PRBUPort");
TEXTCONST(szPICSRULESBYURLPATH,"PRBUPath");
TEXTCONST(szPICSRULESBYURLURL,"PRBUUrl");
TEXTCONST(szPICSRULESSERVICEINFO,"PRServiceInfo");
TEXTCONST(szPICSRULESNUMSERVICEINFO,"PRNumSI");
TEXTCONST(szPICSRULESSIURLNAME,"PRSIURLName");
TEXTCONST(szPICSRULESSIBUREAUURL,"PRSIBureauURL");
TEXTCONST(szPICSRULESSISHORTNAME,"PRSIShortName");
TEXTCONST(szPICSRULESSIRATFILE,"PRSIRatFile");
TEXTCONST(szPICSRULESSIUSEEMBEDDED,"PRSIUseEmbedded");
TEXTCONST(szPICSRULESSIBUREAUUNAVAILABLE,"PRSIBureauUnavailable");
TEXTCONST(szPICSRULESNUMOPTEXTENSIONS,"PRNumOptExt");
TEXTCONST(szPICSRULESOPTEXTNAME,"PROEName");
TEXTCONST(szPICSRULESOPTEXTSHORTNAME,"PROEShortName");
TEXTCONST(szPICSRULESNUMREQEXTENSIONS,"PRNumReqExt");
TEXTCONST(szPICSRULESREQEXTNAME,"PRREName");
TEXTCONST(szPICSRULESREQEXTSHORTNAME,"PRREShortName");
TEXTCONST(szPICSRULESOPTEXTENSION,"PROptExt");
TEXTCONST(szPICSRULESREQEXTENSION,"PRReqExt");
TEXTCONST(szPICSRULESNUMBYURL,"PRNumURLExpressions");
