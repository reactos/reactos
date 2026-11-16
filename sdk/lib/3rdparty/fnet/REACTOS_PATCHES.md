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

### 3. Per socket event callback

FNET only had a global argumentless RX callback and `fnet_socket_poll`. tcpip.sys needs to know when a
particular connection can read, write, accept, finished connecting, or was reset.

* `stack/fnet_stack_config.h`: `FNET_CFG_SOCKET_EVENT_CALLBACK`, default 0 (no change for upstream users).
* `stack/fnet_socket.h`: `fnet_socket_event_callback_t` and `fnet_socket_set_event_callback(s, callback, cookie)`.
  The callback means "state or buffers may have changed"; the application reads IN/OUT/ERR with the existing
  `fnet_socket_poll` on that socket. That covers receive ready, accept ready, FIN, send space, connect done and
  reset without a new event vocabulary.
* `stack/fnet_socket_prv.h`: callback and cookie fields on `fnet_socket_if_t`, plus a small snapshot struct.
* `stack/fnet_socket.c`: the setter; snapshot capture and fire helpers; `_fnet_socket_copy` clears the callback
  so connections in a listener's queue do not inherit the listener's callback.
* `stack/fnet_tcp.c`: in `_fnet_tcp_input`, the callbacks of the socket and of its listener are captured before
  `_fnet_tcp_inputsk` and fired after it. Capturing first means the stack can free the socket meanwhile
  without the callback touching freed memory. Around `_fnet_tcp_slowtimosk` the callback fires only when the
  socket's state or error changed (connect timeout, retransmission or keepalive abort); firing on every 500 ms
  tick woke the glue for every socket for nothing.

Upstream candidate: yes.

### 4. Sending in CLOSE_WAIT (half close)

After the peer's FIN, FNET puts the socket in `SS_CLOSING`. Three places then treated it as closed: sends
failed with `FNET_ERR_NOTCONN`, `fnet_socket_poll` never reported send space, and `_fnet_tcp_close` sent RST
instead of FIN, which can drop a response the peer has not acknowledged yet. FNET's output engine already
handles CLOSE_WAIT to LAST_ACK; only these gates were missing.

* `stack/fnet_tcp.c`: `_fnet_tcp_snd` and its transmit loop allow `FNET_TCP_CS_CLOSE_WAIT`; `_fnet_tcp_close`
  takes the graceful path in CLOSE_WAIT.
* `stack/fnet_socket.c`: `fnet_socket_poll` reports OUT for `SS_CLOSING` while the send side is not shut down
  (the socket layer cannot see TCP state; every FIN we send sets `send_buffer.is_shutdown` first, so this is
  exactly CLOSE_WAIT).

Upstream candidate: yes.

### 5. Close after a half close

`_fnet_tcp_close` aborted with RST whenever the socket was not `SS_CONNECTED`, apart from CLOSE_WAIT (patch 4).
After `shutdown(SD_SEND)` and the peer's FIN, or our FIN in CLOSE_WAIT, the connection is in LAST_ACK, CLOSING
or TIME_WAIT, so a `closesocket` that came before the peer acknowledged our FIN sent RST. The peer then dropped
whatever it had not read yet. Echo style servers that shut down and close at once hit this on every connection
where the close wins the race against the final ACK.

* `stack/fnet_tcp.c`: `_fnet_tcp_close` takes the graceful path in LAST_ACK, CLOSING and TIME_WAIT as well. The
  existing code there already handles them: no new FIN is sent, the socket is marked for deletion and freed
  when the final ACK arrives or TIME_WAIT ends.

Upstream candidate: yes.

### 6. Dropping a duplicate segment from the out of order queue

When a segment in the out of order queue turns out to be entirely old data (a retransmission that covered it
arrived in order first), `_fnet_tcp_add_inpbuf` unlinks it and then subtracts the length of the next queued
segment from `tcpcb_count`. That count is wrong, and when the dropped segment was the last one the next
pointer is NULL, which faults in the kernel. Needs loss and reordering together, so it does not show in the
emulated tests.

* `stack/fnet_tcp.c`: subtract the length of the segment that is dropped.

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
