#ifndef _DOT1X_H
#define _DOT1X_H

#ifdef __cplusplus
extern "C" {
#endif

#include <l2cmn.h>
#include <eaptypes.h>

#if defined(__midl) || defined(__WIDL__)
#define V1_ENUM [v1_enum]
#else
#define V1_ENUM
#endif

#if (NTDDI_VERSION >= NTDDI_VISTA)
typedef V1_ENUM enum _ONEX_NOTIFICATION_TYPE {
    OneXPublicNotificationBase = 0,
    OneXNotificationTypeResultUpdate,
    OneXNotificationTypeAuthRestarted,
    OneXNotificationTypeEventInvalid,
    OneXNumNotifications = OneXNotificationTypeEventInvalid
} ONEX_NOTIFICATION_TYPE /* HACK: WIDL is broken, *PONEX_NOTIFICATION_TYPE */;
#endif

#ifdef __cplusplus
}
#endif

#endif // _DOT1X_H
