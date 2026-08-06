
#pragma once

/*
 * ReactOS ships two d3dhal.h: the kernel display-driver one in
 * sdk/include/ddk, and the DirectX 7 user one in sdk/include/psdk.
 * sdk/include/ddk precedes sdk/include/psdk on the include path, so a plain
 * #include "d3dhal.h" lacks D3DDEVICEDESC_V1/V2/V3.
 *
 * Select it explicitly here. Only
 * modules built with set_wine_module() see this file.
 */
#include <psdk/d3dhal.h>
