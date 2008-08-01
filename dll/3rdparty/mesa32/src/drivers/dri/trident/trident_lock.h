/* XXX tridentGetLock doesn't exist... */

#define LOCK_HARDWARE(tmesa) \
    do { \
        char __ret = 0; \
        DRM_CAS(tmesa->driHwLock, tmesa->hHWContext, \
            DRM_LOCK_HELD | tmesa->hHWContext, __ret); \
    } while (0)

#define UNLOCK_HARDWARE(tmesa) \
    DRM_UNLOCK(tmesa->driFd, tmesa->driHwLock, tmesa->hHWContext)
