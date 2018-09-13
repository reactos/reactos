
#ifndef SITESIDS_H
#define SITESIDS_H

#ifndef URL2SIDAPI
#define URL2SIDAPI __declspec(dllimport)
#endif // ifndef URL2SIDAPI

// defined constants

#define SITE_SID_CACHE_SIZE_HIGH    (2000)
#define SITE_SID_CACHE_SIZE_LOW     (1800)
#define SITE_SID_CACHE_AUTHORITY    SECURITY_INTERNETSITE_AUTHORITY
#define SITE_SID_CACHE_REG_KEY      L"Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\SiteSidCache"

#define SITE_SID_HASH_ALGORITHM     (CALG_MD5)
#define SITE_SID_RID 0x52535452

#ifdef __cplusplus
extern "C"
#endif
NTSTATUS LsapDbInitSiteSidCache();


#endif // ifndef SITESIDS_H