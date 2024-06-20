#ifndef LWIP_PCAPIF_HELPER_H
#define LWIP_PCAPIF_HELPER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct pcapifh_linkstate;

enum pcapifh_link_event {
  PCAPIF_LINKEVENT_UNKNOWN,
  PCAPIF_LINKEVENT_UP,
  PCAPIF_LINKEVENT_DOWN
};

struct pcapifh_linkstate* pcapifh_linkstate_init(char *adapter_name);
enum pcapifh_link_event pcapifh_linkstate_get(struct pcapifh_linkstate* state);
void pcapifh_linkstate_close(struct pcapifh_linkstate* state);

void *pcapifh_alloc_readonly_copy(void *data, size_t len);
void pcapifh_free_readonly_mem(void *data);

void pcapifh_init_npcap(void);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_PCAPIF_HELPER_H */
