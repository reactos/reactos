// MSJ 
// It it works contact Paul DiLascia
// if not you're own your own

#ifndef __WINDOWPLACEMENT_H__
#define __WINDOWPLACEMENT_H__

// CWindowPlacement reads and writes WINDOWPLACEMENT 
// Helper for restoring the app size and placement.
class CTRL_EXT_CLASS CWindowPlacement : public WINDOWPLACEMENT {
public:
   CWindowPlacement();
   ~CWindowPlacement();
   
   // Read/write to app profile
   BOOL GetProfileWP(LPCTSTR lpKeyName);
   void WriteProfileWP(LPCTSTR lpKeyName);

   // Save/restore window pos (from app profile)
   void Save(LPCTSTR lpKeyName, CWnd* pWnd);
   BOOL Restore(LPCTSTR lpKeyName, CWnd* pWnd);

   // Save/restore from archive
   friend CArchive& operator<<(CArchive& ar, const CWindowPlacement& wp);
   friend CArchive& operator>>(CArchive& ar, CWindowPlacement& wp);
private:
	int m_showCmd;
};
  
#endif