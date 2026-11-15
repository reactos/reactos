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

### 1b. Heap unit size vs block header (x64 layout)

Fixing the casts was not enough. `fnet_mempool_unit_header_t` (`size` plus a `next` pointer, packed) is
8 bytes on 32 bit targets and 12 on x64, but `_fnet_heap_init` used an 8 byte unit, and the allocator assumes
the header fits in one unit (user data starts one unit after it). On x64 a split that leaves a one unit free
block makes that block's `ptr` overwrite the first 4 bytes (`size`) of the next block's header.

* `stack/fnet_netbuf.c`: unit size comes from `FNET_CFG_HEAP_ALIGN`, default `FNET_MEMPOOL_ALIGN_8`
  (unchanged for upstream targets). The ReactOS config uses 16 on x64 and 8 on x86.
* `stack/fnet_mempool.c`: `_fnet_mempool_init` fails if the unit is smaller than the header, so this cannot
  come back silently.

Other packed structs with pointers, audited and not patched:

* `fnet_ip4_frag_header_t` and `fnet_ip4_frag_list_t` (`stack/fnet_ip4_prv.h`) overlay received IP headers and
  hold three pointers, which does not fit on x64. Only compiled with `FNET_CFG_IP4_FRAGMENTATION`, which
  ReactOS keeps off (ReactOS reassembles before TCP). The port config has an `#error` if it is turned on for
  x64.
* `_fnet_mempool_init` aligns `struct fnet_mempool` (which holds a pointer) to 4 bytes. Harmless here because
  the heap base comes from pool allocation and is already 8 or 16 byte aligned.
* `fnet_netbuf` data starts 4 bytes into its allocation (a `fnet_uint32_t` reference count precedes it), so
  packet data is 4 byte aligned. Fine on x86 and x64, and protocol headers are read through byte or packed
  accessors.

Upstream candidate: yes.

### 2. Unscaled receive window cap

* `stack/fnet_tcp.h`: `FNET_TCP_MAXWIN` raised from 12 KiB to 0xffff.

FNET clamps the receive buffer to `FNET_TCP_MAXWIN` when the peer does not send a window scale option, and
caps the window in the SYN to it. With 12 KiB, a peer without scaling would get a smaller window than lwIP
gives today. The scale factor is also computed from this value, so the shift becomes smaller for the same
buffer.

Upstream candidate: probably, as a config option.

## Not needed

### 32 bit millisecond clock wrap

The port's clock is `KeQueryInterruptTime() / 10000` truncated to 32 bits, which wraps after 49.7 days.
FNET is safe with it: `_fnet_timer_poll`, ARP, PMTU and link checks all use unsigned `now - start` against a
timeout, and TCP uses countdown tick counters, not absolute times. The absolute seconds users (`fnet_time`,
IPv6 address lifetimes) are compiled out.

### Abortive close

No patch needed: `fnet_socket_close` with `SO_LINGER` on and a zero timeout calls `_fnet_tcp_abort_socket`,
which sends RST (`stack/fnet_tcp.c`, `_fnet_tcp_close`).
