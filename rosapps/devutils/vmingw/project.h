/********************************************************************
*	Module:	project.h. This is part of Visual-MinGW.
*
*	License:	Visual-MinGW is covered by GNU General Public License, 
*			Copyright (C) 2001  Manu B.
*			See license.htm for more details.
*
********************************************************************/
#ifndef PROJECT_H
#define PROJECT_H

#include "winui.h"
#include "main.h"
#include "process.h"

#define BUILD_STATLIB		0
#define BUILD_DLL		1
#define BUILD_EXE		2
#define BUILD_GUIEXE		3
#define LANGC	0
#define LANGCPP	1

class CProject;
class CMakefile;

bool CheckFile(CFileItem * file);

class CGeneralDlg : public CDlgBase
{
	public:
	CGeneralDlg();
	virtual ~CGeneralDlg();

	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	protected:

	private:   
	CProject *pProject;
	CMakefile *pMakefile;
	HWND hStatLib;
	HWND hDll;
	HWND hConsole;
	HWND hGuiExe;
	HWND hDbgSym;
	HWND hLangC;
	HWND hLangCpp;
	HWND hMkfName;
	HWND hMkfDir;
	HWND hUserMkf;
	HWND hTgtName;
	HWND hTgtDir;
};

class CCompilerDlg : public CDlgBase
{
	public:
	CCompilerDlg();
	virtual ~CCompilerDlg();

	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	protected:

	private:   
	CProject *pProject;
	CMakefile *pMakefile;
	HWND hCppFlags;
	HWND hWarning;
	HWND hOptimiz;
	HWND hCFlags;
	HWND hIncDirs;
};

class CZipDlg : public CDlgBase
{
	public:
	CZipDlg();
	virtual ~CZipDlg();

	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	protected:

	private:   
	CProject *pProject;
	CMakefile *pMakefile;
	HWND hZipDir;
	HWND hZipFlags;
};

class CLinkerDlg : public CDlgBase
{
	public:
	CLinkerDlg();
	virtual ~CLinkerDlg();

	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	protected:

	private:   
	CProject *pProject;
	CMakefile *pMakefile;
	HWND hLdStrip;
	HWND hLdOpts;
	HWND hLdLibs;
	HWND hLibsDirs;
};

class COptionsDlg : public CTabbedDlg
{
	public:
	COptionsDlg();
	virtual ~COptionsDlg();

	BOOL EndDlg(int nResult);
	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL	OnInitDialog(HWND hwndFocus, LPARAM lInitParam);
	BOOL OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	protected:

	private:   
	CProject *pProject;
	CMakefile *pMakefile;
	CGeneralDlg	GeneralDlg;
	CCompilerDlg	CompilerDlg;
	CLinkerDlg		LinkerDlg;
	CZipDlg		ZipDlg;
};

class CNewModuleDlg : public CDlgBase
{
	public:
	CNewModuleDlg();
	virtual ~CNewModuleDlg();

	virtual LRESULT CALLBACK CDlgProc(UINT Message, WPARAM wParam, LPARAM lParam);

	protected:

	private:   
	CProject *pProject;
};

class CCompiler : public CIniFile
{
	public:
	CCompiler();
	~CCompiler();

	bool LoadData(char * fullpath);

	char make[64];

	char cc[16];
	char cFlags[64];
	char ldFlags[64];
	char wres[16];

	char debug[16];
	char test[16];

	protected:

	private:
};

class CMakefile
{
	public:
	CMakefile();
	~CMakefile();

	void Init(void);
	bool SwitchCurrentDir(void);
	void GetFullPath(char * prjFileName, WORD offset, char * name);
	void Build(CProjectView * Tree, CProcess* Process);
	void SrcList2Buffers(CProjectView * Tree);
	void Write(void);

	// File.
	char szFileName[MAX_PATH];
	WORD nFileOffset;
	char mkfDir[MAX_PATH];

	// Compiler dependent.
	char cc[4];
	char make[64];
	char wres[16];
	char test[16];

	char target[64];
	char tgtDir[MAX_PATH];
	UINT buildWhat;
	bool debug;
	UINT lang;

	// Compiler data.
	char cppFlags[256];
	char warning[64];
	char optimize[64];
	char cFlags[64];
	char incDirs[256];

	// Linker data.
	char ldStrip[32];
	char ldOpts[64];
	char ldLibs[64];
	char libDirs[256];

	// Archiver.
	char arFlags[64];

	protected:

	private:
	// Buffers.
	char objFile[64];
	char srcBuf [1024];
	char objBuf [1024];
	char resBuf [1024];
	char depBuf [256];
};

class CProject //: public CIniFile
{
	public:
	CProject();
	~CProject();

	bool	NoProject(void);
	int 	CloseDecision(void);
	bool	New(char * fileName, WORD fileOffset);
	bool	Open(char * fileName, WORD fileOffset);

	bool RelativeToAbsolute(char * relativePath);
	bool	AddFiles(void);

	bool OptionsDlg(void);
	bool NewModuleDlg(void);
	bool NewModule(char * srcFile, bool createHeader);

	void	ZipSrcs(void);
	void	Explore(HWND hwnd);

	void	Build(void);
	void	RebuildAll(void);
	void	RunTarget(void);
	void	MakeClean(void);
	void	BuildMakefile(void);

	bool SwitchCurrentDir(void);

	CMakefile		Makefile;

	/* May be private members */
	int 		numFiles;
	bool		loaded;
	bool 		modified;
	bool		buildMakefile;
	int SavePrjFile(int decision);

	char		szFileName[MAX_PATH];
	WORD 	nFileOffset;
	WORD	nFileExtension;
	char		szDirBuffer[MAX_PATH];

	char zipDir[MAX_PATH];
	char zipFlags[256];

	char		compilerName[64];

	protected:

	private:   
	void Reset();

CIniFile PrjFile;
	COptionsDlg	_OptionsDlg;
	CNewModuleDlg	_NewModuleDlg;

	int	prjVer;
};

#endif
