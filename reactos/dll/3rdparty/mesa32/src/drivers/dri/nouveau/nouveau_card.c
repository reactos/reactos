
#include "nouveau_card.h"
#include "nouveau_reg.h"
#include "nouveau_drm.h"
#include "nouveau_card_list.h"


nouveau_card* nouveau_card_lookup(uint32_t device_id)
{
	int i;
	for(i=0;i<sizeof(nouveau_card_list)/sizeof(nouveau_card)-1;i++)
		if (nouveau_card_list[i].id==(device_id&0xffff))
			return &(nouveau_card_list[i]);
	return NULL;
}


