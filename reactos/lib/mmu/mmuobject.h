#ifndef _LIBMMU_MMUOBJECT_H
#define _LIBMMU_MMUOBJECT_H

MmuPageCallback callback;
void mmuaddpage(void *va, ppc_map_info_t *info, int count);
void mmudelpage(void *va);
void mmusetvsid(int start, int end, int vsid);
int mmunitest();

#endif/*_LIBMMU_MMUOBJECT_H*/
