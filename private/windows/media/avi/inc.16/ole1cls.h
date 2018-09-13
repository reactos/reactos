/* This file is the master definition of all GUIDs for OLE1 classes.
   
   All such GUIDs are of the form:

       0003xxxx-0000-0000-C000-000000000046

    The last parameter to DEFINE_OLE1GUID is the old 1.0 class name,
    i.e., its key in the registration database.

    Do not remove or change GUIDs.

    Do not add anything to this file except comments and DEFINE_OLE1GUID macros.
*/

#ifndef DEFINE_OLE1GUID
#define DEFINE_OLE1GUID(a,b,c,d,e) DEFINE_OLEGUID (a,b,c,d)
#endif
   
DEFINE_OLE1GUID(CLSID_ExcelWorksheet,   0x00030000, 0, 0, "ExcelWorksheet");
DEFINE_OLE1GUID(CLSID_ExcelChart,       0x00030001, 0, 0, "ExcelChart");
DEFINE_OLE1GUID(CLSID_ExcelMacrosheet,  0x00030002, 0, 0, "ExcelMacrosheet");
DEFINE_OLE1GUID(CLSID_WordDocument,     0x00030003, 0, 0, "WordDocument");
DEFINE_OLE1GUID(CLSID_MSPowerPoint,     0x00030004, 0, 0, "MSPowerPoint");
DEFINE_OLE1GUID(CLSID_MSPowerPointSho,  0x00030005, 0, 0, "MSPowerPointSho");
DEFINE_OLE1GUID(CLSID_MSGraph,          0x00030006, 0, 0, "MSGraph");
DEFINE_OLE1GUID(CLSID_MSDraw,               0x00030007, 0, 0, "MSDraw");
DEFINE_OLE1GUID(CLSID_Note_It,          0x00030008, 0, 0, "Note-It");
DEFINE_OLE1GUID(CLSID_WordArt,          0x00030009, 0, 0, "WordArt");
DEFINE_OLE1GUID(CLSID_PBrush,               0x0003000a, 0, 0, "PBrush");
DEFINE_OLE1GUID(CLSID_Equation,         0x0003000b, 0, 0, "Equation");
DEFINE_OLE1GUID(CLSID_Package,          0x0003000c, 0, 0, "Package");
DEFINE_OLE1GUID(CLSID_SoundRec,         0x0003000d, 0, 0, "SoundRec");
DEFINE_OLE1GUID(CLSID_MPlayer,          0x0003000e, 0, 0, "MPlayer");

/* test apps */
DEFINE_OLE1GUID(CLSID_ServerDemo,       0x0003000f, 0, 0, "ServerDemo");
DEFINE_OLE1GUID(CLSID_Srtest,               0x00030010, 0, 0, "Srtest");
DEFINE_OLE1GUID(CLSID_SrtInv,               0x00030011, 0, 0, "SrtInv");
DEFINE_OLE1GUID(CLSID_OleDemo,          0x00030012, 0, 0, "OleDemo");

/* External ISVs */
// Coromandel / Dorai Swamy / 718-793-7963
DEFINE_OLE1GUID(CLSID_CoromandelIntegra,    0x00030013, 0, 0, "CoromandelIntegra");
DEFINE_OLE1GUID(CLSID_CoromandelObjServer,0x00030014, 0, 0, "CoromandelObjServer");

// 3-d Visions Corp / Peter Hirsch / 310-325-1339
DEFINE_OLE1GUID(CLSID_StanfordGraphics, 0x00030015, 0, 0, "StanfordGraphics");

// Deltapoint / Nigel Hearne / 408-648-4000
DEFINE_OLE1GUID(CLSID_DGraphCHART,          0x00030016, 0, 0, "DGraphCHART");
DEFINE_OLE1GUID(CLSID_DGraphDATA,           0x00030017, 0, 0, "DGraphDATA");

// Corel / Richard V. Woodend / 613-728-8200 x1153
DEFINE_OLE1GUID(CLSID_PhotoPaint,           0x00030018, 0, 0, "PhotoPaint");
DEFINE_OLE1GUID(CLSID_CShow,                    0x00030019, 0, 0, "CShow");
DEFINE_OLE1GUID(CLSID_CorelChart,           0x0003001a, 0, 0, "CorelChart");
DEFINE_OLE1GUID(CLSID_CDraw,                    0x0003001b, 0, 0, "CDraw");

// Inset Systems / Mark Skiba / 203-740-2400
DEFINE_OLE1GUID(CLSID_HJWIN1_0,             0x0003001c, 0, 0, "HJWIN1.0");

// Mark V Systems / Mark McGraw / 818-995-7671
DEFINE_OLE1GUID(CLSID_ObjMakerOLE,          0x0003001d, 0, 0, "ObjMakerOLE");

// IdentiTech / Mike Gilger / 407-951-9503
DEFINE_OLE1GUID(CLSID_FYI,                      0x0003001e, 0, 0, "FYI");
DEFINE_OLE1GUID(CLSID_FYIView,                  0x0003001f, 0, 0, "FYIView");

// Inventa Corporation / Balaji Varadarajan / 408-987-0220
DEFINE_OLE1GUID(CLSID_Stickynote,       0x00030020, 0, 0, "Stickynote");

// ShapeWare Corp. / Lori Pearce / 206-467-6723
DEFINE_OLE1GUID(CLSID_ShapewareVISIO10, 0x00030021, 0, 0, "ShapewareVISIO10");
DEFINE_OLE1GUID(CLSID_ImportServer,     0x00030022, 0, 0, "ImportServer");


// test app SrTest
DEFINE_OLE1GUID(CLSID_SrvrTest,          0x00030023, 0, 0, "SrvrTest");

// Special clsid for when a 1.0 client pastes an embedded object
// that is a link.
// **This CLSID is obsolete. Do not reuse number.
//DEFINE_OLE1GUID(CLSID_10EmbedObj,        0x00030024, 0, 0, "OLE2_Embedded_Link");

// test app ClTest.  Doesn't really work as a server but is in reg db
DEFINE_OLE1GUID(CLSID_ClTest,            0x00030025, 0, 0, "Cltest");

// Microsoft ClipArt Gallery   Sherry Larsen-Holmes
DEFINE_OLE1GUID(CLSID_MS_ClipArt_Gallery,0x00030026, 0, 0, "MS_ClipArt_Gallery");

// Microsoft Project  Cory Reina
DEFINE_OLE1GUID(CLSID_MSProject,         0x00030027, 0, 0, "MSProject");

// Microsoft Works Chart
DEFINE_OLE1GUID(CLSID_MSWorksChart,      0x00030028, 0, 0, "MSWorksChart");

// Microsoft Works Spreadsheet
DEFINE_OLE1GUID(CLSID_MSWorksSpreadsheet,0x00030029, 0, 0, "MSWorksSpreadsheet");

// AFX apps - Dean McCrory
DEFINE_OLE1GUID(CLSID_MinSvr,            0x0003002A, 0, 0, "MinSvr");
DEFINE_OLE1GUID(CLSID_HierarchyList,     0x0003002B, 0, 0, "HierarchyList");
DEFINE_OLE1GUID(CLSID_BibRef,            0x0003002C, 0, 0, "BibRef");
DEFINE_OLE1GUID(CLSID_MinSvrMI,          0x0003002D, 0, 0, "MinSvrMI");
DEFINE_OLE1GUID(CLSID_TestServ,          0x0003002E, 0, 0, "TestServ");

// Ami Pro
DEFINE_OLE1GUID(CLSID_AmiProDocument,    0x0003002F, 0, 0, "AmiProDocument");

// WordPerfect Presentations For Windows
DEFINE_OLE1GUID(CLSID_WPGraphics,       0x00030030, 0, 0, "WPGraphics");
DEFINE_OLE1GUID(CLSID_WPCharts,         0x00030031, 0, 0, "WPCharts");


// MicroGrafx Charisma
DEFINE_OLE1GUID(CLSID_Charisma,         0x00030032, 0, 0, "Charisma");
DEFINE_OLE1GUID(CLSID_Charisma_30,      0x00030033, 0, 0, "Charisma_30");
DEFINE_OLE1GUID(CLSID_CharPres_30,      0x00030034, 0, 0, "CharPres_30");

// MicroGrafx Draw
DEFINE_OLE1GUID(CLSID_Draw,             0x00030035, 0, 0, "Draw");

// MicroGrafx Designer
DEFINE_OLE1GUID(CLSID_Designer_40,      0x00030036, 0, 0, "Designer_40");


#undef DEFINE_OLE1GUID

/* as we discover OLE 1 servers we will add them to the end of this list;
   there is room for 64K of them!
*/
