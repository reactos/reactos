#ifndef __INCLUDE_SERVICES_SERVICES_H
#define __INCLUDE_SERVICES_SERVICES_H

#define SCM_OPEN_MANAGER              (1)
#define SCM_LOCK_MANAGER              (10)
#define SCM_UNLOCK_MANAGER            (11)

#define SCM_CREATE_SERVICE            (2)
#define SCM_OPEN_SERVICE              (3)
#define SCM_CHANGE_CONFIG_SERVICE     (4)
#define SCM_CONTROL_SERVICE           (5)
#define SCM_DELETE_SERVICE            (6)
#define SCM_SERVICES_STATUS           (7)
#define SCM_GET_DISPLAY_NAME_SERVICE  (8)
#define SCM_GET_KEY_NAME_SERVICE      (9)
#define SCM_QUERY_CONFIG_SERVICE      (12)
#define SCM_QUERY_LOCK_STATUS_SERVICE (13)
#define SCM_QUERY_STATUS_SERVICE      (14)
#define SCM_START_SERVICE             (15)

#define SCM_REGISTER_CHANDLER         (16)
#define SCM_SET_STATUS_SERVICE        (17)
#define SCM_START_CDISPATCHER         (18)

typedef struct
{
   ULONG Type;
} SCM_API_REQUEST;

typedef struct
{
   ULONG Status;
} SCM_API_REPLY;

#endif /* __INCLUDE_SERVICES_SERVICES_H */
