/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/*
**  Set of prototypes and stuctures for  process and thread handling
*/

LPPD CreatePd(HPID hpid);
VOID DestroyPd(LPPD lppd, BOOL fDestroyPrecious);
LPPD GetLppdHead( void );
LPPD LppdOfHpid(HPID hpid);
LPPD LppdOfIpid(UINT ipid);
LPPD ValidLppdOfIpid(UINT ipid);
BOOL GetFirstValidPDTD(LPPD *plppd, LPTD *plptd);
LPTD CreateTd(LPPD lppd, HTID htid);
VOID DestroyTd(LPTD lptd);
LPTD LptdOfLppdHtid(LPPD lppd, HTID htid);
LPTD LptdOfLppdItid(LPPD lppd, UINT itid);
VOID SetIpid(int);
VOID RecycleIpid1(void);
VOID SetTdInfo(LPTD lptd);
VOID SetPdInfo(LPPD lppd);
