#ifndef __DIRWALK_H__
#define __DIRWALK_H__

////////////////////////////////////////////////
// CSplitPath
////////////////////////////////////////////////
class CTRL_EXT_CLASS CSplitPath
{
public:
	CSplitPath(LPCTSTR pszPath);
	CSplitPath();
	virtual ~CSplitPath();
// operations
public:
	void Split(LPCTSTR pszPath);
	void Make();
// attributes
public:
	CString GetPath() const;
	CString GetDrive() const;
	CString GetDir() const;
	CString GetFileName() const;
	CString GetExt() const;
	void SetDrive(LPCTSTR pszDrive);
	void SetDir(LPCTSTR pszDir);
	void SetFileName(LPCTSTR pszFileName);
	void SetExt(LPCTSTR pszExt);
protected:
	void Init();
private:
	TCHAR m_szPath[MAX_PATH];
	TCHAR m_szDrive[_MAX_DRIVE];
	TCHAR m_szDir[_MAX_DIR];
	TCHAR m_szFname[_MAX_FNAME];
	TCHAR m_szExt[_MAX_EXT];
};

inline CSplitPath::CSplitPath()
{
	Init();
}

inline CSplitPath::CSplitPath(LPCTSTR pszPath)
{
	Init();
	Split(pszPath);
}

inline void CSplitPath::Init()
{
	m_szPath[0] = 0;
	m_szDrive[0] = 0;
	m_szDir[0] = 0;
	m_szFname[0] = 0;
	m_szExt[0] = 0;
}

inline CSplitPath::~CSplitPath()
{

}

inline CString CSplitPath::GetPath() const
{
	return m_szPath;
}

inline CString CSplitPath::GetDrive() const
{
	return m_szDrive;
}

inline CString CSplitPath::GetDir() const
{
	return m_szDir;
}

inline CString CSplitPath::GetFileName() const
{
	return m_szFname;
}

inline CString CSplitPath::GetExt() const
{
	return m_szExt;
}

inline void CSplitPath::SetDrive(LPCTSTR pszDrive)
{
	lstrcpy(m_szDrive,pszDrive);
}

inline void CSplitPath::SetDir(LPCTSTR pszDir)
{
	lstrcpy(m_szDir,pszDir);
}

inline void CSplitPath::SetFileName(LPCTSTR pszFileName)
{
	lstrcpy(m_szFname,pszFileName);
}

inline void CSplitPath::SetExt(LPCTSTR pszExt)
{
	lstrcpy(m_szExt,pszExt);
}

inline void CSplitPath::Split(LPCTSTR pszPath)
{
	_tsplitpath(pszPath,m_szDrive,m_szDir,m_szFname,m_szExt);
}

inline void CSplitPath::Make()
{
	_tmakepath(m_szPath,m_szDrive,m_szDir,m_szFname,m_szExt);
}

#define ARRAY_SIZE(A)  (sizeof(A) / sizeof((A)[0]))

#endif //__DIRWALK_H__
