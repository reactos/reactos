/* pref.h
 *
 * Copyright (c) 1992-2001 by Mike Gleason.
 * All rights reserved.
 * 
 */

#if defined(WIN32) || defined(_WINDOWS)
#	define kFirewallPrefFileName			"firewall.txt"
#	define kGlobalFirewallPrefFileName		"..\\..\\firewall.txt"
#	define kGlobalFixedFirewallPrefFileName		"..\\..\\firewall_fixed.txt"
#	define kGlobalPrefFileName			"..\\..\\prefs_v3.txt"
#	define kGlobalFixedPrefFileName			"..\\..\\prefs_v3_fixed.txt"
#	define kPrefFileName				"prefs_v3.txt"
#	define kPrefFileNameV2				"prefs"
#	define kFirstFileName				"init_v3.txt"
#else
#	define kFirewallPrefFileName			"firewall"
#	define kGlobalFirewallPrefFileName		"/etc/ncftp.firewall"
#	define kGlobalFixedFirewallPrefFileName		"/etc/ncftp.firewall.fixed"
#	define kGlobalPrefFileName			"/etc/ncftp.prefs_v3"
#	define kGlobalFixedPrefFileName			"/etc/ncftp.prefs_v3.fixed"
#	define kPrefFileName				"prefs_v3"
#	define kPrefFileNameV2				"prefs"
#	define kFirstFileName				"init_v3"
#endif

#define kOpenSelectedBookmarkFileName		"bm2open"

typedef void (*PrefProc)(int i, const char *const, FILE *const fp);
typedef struct PrefOpt {
	const char *varname;
	PrefProc proc;
	int visible;
} PrefOpt;

#define kPrefOptObselete (-1)
#define kPrefOptInvisible 0
#define kPrefOptVisible 1

#define PREFOBSELETE (PrefProc) 0, kPrefOptObselete,

/* pref.c */
void SetAnonPass(int, const char *const, FILE *const);
void SetAutoAscii(int t, const char *const val, FILE *const fp);
void SetAutoResume(int, const char *const, FILE *const);
void SetAutoSaveChangesToExistingBookmarks(int t, const char *const val, FILE *const fp);
void SetConfirmClose(int, const char *const, FILE *const);
void SetConnTimeout(int, const char *const, FILE *const);
void SetCtrlTimeout(int, const char *const, FILE *const);
void SetLogSize(int t, const char *const val, FILE *const fp);
void SetNoAds(int t, const char *const val, FILE *const fp);
void SetOneTimeMessages(int t, const char *const val, FILE *const);
void SetPager(int, const char *const, FILE *const);
void SetPassive(int, const char *const, FILE *const);
void SetProgressMeter(int, const char *const, FILE *const);
void SetRedialDelay(int t, const char *const val, FILE *const fp);
void SetSavePasswords(int, const char *const, FILE *const);
void SetSOBufsize(int t, const char *const val, FILE *const fp);
void SetXferTimeout(int, const char *const, FILE *const);
void SetXtTitle(int, const char *const, FILE *const);
void Set(const char *const, const char *const);
void ProcessPrefsFile(FILE *const fp);
void LoadPrefs(void);
void InitPrefs(void);
void PostInitPrefs(void);
void SavePrefs(void);
void WriteDefaultFirewallPrefs(FILE *);
void ProcessFirewallPrefFile(FILE *);
void LoadFirewallPrefs(int);
void CheckForNewV3User(void);
int HasSeenOneTimeMessage(const char *const msg);
void SetSeenOneTimeMessage(const char *const msg);
int OneTimeMessage(const char *const msg);
