/* cmds.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

/* cmds.c */
int PromptForBookmarkName(BookmarkPtr);
void CurrentURL(char *, size_t, int);
void FillBookmarkInfo(BookmarkPtr);
void SaveCurrentAsBookmark(void);
void SaveUnsavedBookmark(void);
void BookmarkCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void CatCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void PrintResp(LineListPtr);
int nFTPChdirAndGetCWD(const FTPCIPtr, const char *, const int);
int Chdirs(FTPCIPtr cip, const char *const cdCwd);
void BGStartCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void ChdirCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void ChmodCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void CloseCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void DebugCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void DeleteCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void EchoCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void InitTransferType(void);
void GetCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void HelpCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void HostsCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void JobsCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void ListCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalChdirCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalListCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalChmodCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalMkdirCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalPageCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalRenameCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalRmCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalRmdirCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LocalPwdCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void LookupCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void MkdirCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void MlsCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
int DoOpen(void);
void OpenCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void PageCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void PutCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void PwdCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void QuitCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void QuoteCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void RGlobCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void RenameCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void RmdirCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void RmtHelpCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void SetCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void ShellCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void SiteCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void SpoolGetCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void SpoolPutCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void SymlinkCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void TypeCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void UmaskCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
void VersionCmd(const int, const char **const, const CommandPtr, const ArgvInfoPtr);
