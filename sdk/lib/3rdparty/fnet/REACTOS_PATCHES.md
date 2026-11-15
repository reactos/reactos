# ReactOS changes to FNET

Upstream: https://github.com/butok/FNET, tag v4.7.1, commit 1a4d001f233ab1eae4c484205b77f71848205f00.

Imported: `fnet_license.txt`, `fnet_stack/fnet.h`, `fnet_stack/fnet_config.h`, `fnet_stack/stack/`, `fnet_stack/port/compiler/`,
`fnet_stack/port/cpu/{fnet_cpu.c,fnet_cpu.h,fnet_cpu_config.h}`, `fnet_stack/service/fnet_service.*`,
`fnet_stack/service/fnet_service_prv.h` and the service headers that `fnet.h` and `fnet_config.h` include.
Vendor CPU ports, network drivers, service sources, demos, docs and tools are left out.

The ReactOS port (config, CPU types, heap, timer, worker thread, debug output) lives in
`drivers/network/tcpip/fnet_port/` and is not part of this directory.

Every changed line carries a `/* ReactOS */` comment. Regenerate the diff with:

    diff -ru <upstream>/fnet_stack/stack sdk/lib/3rdparty/fnet/fnet_stack/stack

## Applied

### 1. Pointer sized integers for address arithmetic (x64)

FNET stores pointers in `fnet_uint32_t` and `fnet_int32_t` in several places. On x64 kernel addresses do not
fit, so these truncate. A new macro `FNET_UINTPTR_T` defaults to `fnet_uint32_t` (no change for upstream
targets) and the ReactOS config sets it to a pointer sized type.

* `stack/fnet_stdlib.h`: define `FNET_UINTPTR_T`; `FNET_ALIGN_DIV` uses it.
* `stack/fnet_tcp.h`: `FNET_TCP_GETUCHAR`, `FNET_TCP_GETUSHORT`, `FNET_TCP_GETULONG` use it. These read TCP
  header and option fields and are used about 130 times.
* `stack/fnet_mempool.h`: `fnet_mempool_desc_t` is `FNET_UINTPTR_T` (it holds a `struct fnet_mempool *`).
* `stack/fnet_mempool.c`: 10 lines of block header arithmetic.
* `stack/fnet_timer_prv.h`, `stack/fnet_timer.c`: timer handler cookie is `FNET_UINTPTR_T` (callers pass
  `fnet_netif_t *` and `fnet_arp_if_t *`).
* `stack/fnet_arp.c`, `stack/fnet_eth.c`, `stack/fnet_netif.c`, `stack/fnet_ip4.c`, `stack/fnet_tcp.c`: timer
  handler signatures and the cookie casts at the call sites.

Upstream candidate: yes.

## Not needed

### 32 bit millisecond clock wrap

The port's clock is `KeQueryInterruptTime() / 10000` truncated to 32 bits, which wraps after 49.7 days.
FNET is safe with it: `_fnet_timer_poll`, ARP, PMTU and link checks all use unsigned `now - start` against a
timeout, and TCP uses countdown tick counters, not absolute times. The absolute seconds users (`fnet_time`,
IPv6 address lifetimes) are compiled out.

### Abortive close

No patch needed: `fnet_socket_close` with `SO_LINGER` on and a zero timeout calls `_fnet_tcp_abort_socket`,
which sends RST (`stack/fnet_tcp.c`, `_fnet_tcp_close`).
