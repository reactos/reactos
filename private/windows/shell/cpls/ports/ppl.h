#ifndef PLL_H
#define PLL_H

#include "ports.h"
#include "pp.h"

void PplConstruct(void);
void PplDestruct(void);

POUR_PROP_PARAMS PplAddParams(DWORD DevInst);
BOOL             PplRemoveParams(POUR_PROP_PARAMS Params);                           

#endif // PLL_H
