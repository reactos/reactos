#ifndef _ROSRTL_REGISTRY_H_
#define _ROSRTL_REGISTRY_H_

#ifdef __cplusplus
extern "C" {
#endif

NTSTATUS NTAPI RosReadRegistryValue( IN  PUNICODE_STRING Key,
				     IN  PUNICODE_STRING Value,
				     OUT PUNICODE_STRING Result );

#ifdef __cplusplus
};
#endif

#endif/*_ROSRTL_REGISTRY_H_*/
