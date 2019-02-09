#include <stubs.h>

// These are here so we can forward the decorated export functions.
// Without forwarding, we'll lose the decoration
// See:
// https://jira.reactos.org/browse/CORE-8462
// https://jira.reactos.org/browse/CORE-15665

int __stdcall PfAddFiltersToInterface(long a0, long a1, long a2, long a3, long a4, long a5)
{
	DbgPrint("WARNING: calling stub PfAddFiltersToInterface(0x%lx,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3, (long)a4, (long)a5);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfAddGlobalFilterToInterface(long a0, long a1)
{
	DbgPrint("WARNING: calling stub PfAddGlobalFilterToInterface(0x%lx,0x%lx)\n", (long)a0, (long)a1);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfBindInterfaceToIPAddress(long a0, long a1, long a2)
{
	DbgPrint("WARNING: calling stub PfBindInterfaceToIPAddress(0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfBindInterfaceToIndex(long a0, long a1, long a2, long a3)
{
	DbgPrint("WARNING: calling stub PfBindInterfaceToIndex(0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfCreateInterface(long a0, long a1, long a2, long a3, long a4, long a5)
{
	DbgPrint("WARNING: calling stub PfCreateInterface(0x%lx,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3, (long)a4, (long)a5);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfDeleteInterface(long a0)
{
	DbgPrint("WARNING: calling stub PfDeleteInterface(0x%lx)\n", (long)a0);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfDeleteLog()
{
	DbgPrint("WARNING: calling stub PfDeleteLog()\n");
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfGetInterfaceStatistics(long a0, long a1, long a2, long a3)
{
	DbgPrint("WARNING: calling stub PfGetInterfaceStatistics(0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfMakeLog(long a0)
{
	DbgPrint("WARNING: calling stub PfMakeLog(0x%lx)\n", (long)a0);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfRebindFilters(long a0, long a1)
{
	DbgPrint("WARNING: calling stub PfRebindFilters(0x%lx,0x%lx)\n", (long)a0, (long)a1);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfRemoveFilterHandles(long a0, long a1, long a2)
{
	DbgPrint("WARNING: calling stub PfRemoveFilterHandles(0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfRemoveFiltersFromInterface(long a0, long a1, long a2, long a3, long a4)
{
	DbgPrint("WARNING: calling stub PfRemoveFiltersFromInterface(0x%lx,0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3, (long)a4);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfRemoveGlobalFilterFromInterface(long a0, long a1)
{
	DbgPrint("WARNING: calling stub PfRemoveGlobalFilterFromInterface(0x%lx,0x%lx)\n", (long)a0, (long)a1);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfSetLogBuffer(long a0, long a1, long a2, long a3, long a4, long a5, long a6)
{
	DbgPrint("WARNING: calling stub PfSetLogBuffer(0x%lx,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3, (long)a4, (long)a5, (long)a6);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfTestPacket(long a0, long a1, long a2, long a3, long a4)
{
	DbgPrint("WARNING: calling stub PfTestPacket(0x%lx,0x%lx,0x%lx,0x%lx,0x%lx)\n", (long)a0, (long)a1, (long)a2, (long)a3, (long)a4);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

int __stdcall PfUnBindInterface(long a0)
{
	DbgPrint("WARNING: calling stub PfUnBindInterface(0x%lx)\n", (long)a0);
	__wine_spec_unimplemented_stub("iphlpapi.dll", __FUNCTION__);
	return 0;
}

