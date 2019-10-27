/**
 * @defgroup lwip lwIP
 *
 * @defgroup infrastructure Infrastructure
 * 
 * @defgroup api APIs
 * lwIP provides three Application Program's Interfaces (APIs) for programs
 * to use for communication with the TCP/IP code:
 * - low-level "core" / "callback" or @ref callbackstyle_api.
 * - higher-level @ref sequential_api.
 * - BSD-style @ref socket.
 * 
 * The raw TCP/IP interface allows the application program to integrate
 * better with the TCP/IP code. Program execution is event based by
 * having callback functions being called from within the TCP/IP
 * code. The TCP/IP code and the application program both run in the same
 * thread. The sequential API has a much higher overhead and is not very
 * well suited for small systems since it forces a multithreaded paradigm
 * on the application.
 * 
 * The raw TCP/IP interface is not only faster in terms of code execution
 * time but is also less memory intensive. The drawback is that program
 * development is somewhat harder and application programs written for
 * the raw TCP/IP interface are more difficult to understand. Still, this
 * is the preferred way of writing applications that should be small in
 * code size and memory usage.
 * 
 * All APIs can be used simultaneously by different application
 * programs. In fact, the sequential API is implemented as an application
 * program using the raw TCP/IP interface.
 * 
 * Do not confuse the lwIP raw API with raw Ethernet or IP sockets.
 * The former is a way of interfacing the lwIP network stack (including
 * TCP and UDP), the latter refers to processing raw Ethernet or IP data
 * instead of TCP connections or UDP packets.
 * 
 * Raw API applications may never block since all packet processing
 * (input and output) as well as timer processing (TCP mainly) is done
 * in a single execution context.
 *
 * @defgroup callbackstyle_api "raw" APIs
 * @ingroup api
 * Non thread-safe APIs, callback style for maximum performance and minimum
 * memory footprint.
 * Program execution is driven by callbacks functions, which are then
 * invoked by the lwIP core when activity related to that application
 * occurs. A particular application may register to be notified via a
 * callback function for events such as incoming data available, outgoing
 * data sent, error notifications, poll timer expiration, connection
 * closed, etc. An application can provide a callback function to perform
 * processing for any or all of these events. Each callback is an ordinary
 * C function that is called from within the TCP/IP code. Every callback
 * function is passed the current TCP or UDP connection state as an
 * argument. Also, in order to be able to keep program specific state,
 * the callback functions are called with a program specified argument
 * that is independent of the TCP/IP state.
 * The raw API (sometimes called native API) is an event-driven API designed
 * to be used without an operating system that implements zero-copy send and
 * receive. This API is also used by the core stack for interaction between
 * the various protocols. It is the only API available when running lwIP
 * without an operating system.
 * 
 * @defgroup sequential_api Sequential-style APIs
 * @ingroup api
 * Sequential-style APIs, blocking functions. More overhead, but can be called
 * from any thread except TCPIP thread.
 * The sequential API provides a way for ordinary, sequential, programs
 * to use the lwIP stack. It is quite similar to the BSD socket API. The
 * model of execution is based on the blocking open-read-write-close
 * paradigm. Since the TCP/IP stack is event based by nature, the TCP/IP
 * code and the application program must reside in different execution
 * contexts (threads).
 * 
 * @defgroup socket Socket API
 * @ingroup api
 * BSD-style socket API.\n
 * Thread-safe, to be called from non-TCPIP threads only.\n
 * Can be activated by defining @ref LWIP_SOCKET to 1.\n
 * Header is in posix/sys/socket.h\n
 * The socket API is a compatibility API for existing applications,
 * currently it is built on top of the sequential API. It is meant to
 * provide all functions needed to run socket API applications running
 * on other platforms (e.g. unix / windows etc.). However, due to limitations
 * in the specification of this API, there might be incompatibilities
 * that require small modifications of existing programs.
 * 
 * @defgroup netifs NETIFs
 * 
 * @defgroup apps Applications
 */

/**
 * @mainpage Overview
 * @verbinclude "README"
 */

/**
 * @page upgrading Upgrading
 * @verbinclude "UPGRADING"
 */

/**
 * @page changelog Changelog
 *
 * 2.1.0
 * -----
 * * Support TLS via new @ref altcp_api connection API (https, smtps, mqtt over TLS)
 * * Switch to cmake as the main build system (Makefile file lists are still
 *   maintained for now)
 * * Improve IPv6 support: support address scopes, support stateless DHCPv6, bugfixes
 * * Add debug helper asserts to ensure threading/locking requirements are met
 * * Add sys_mbox_trypost_fromisr() and tcpip_callbackmsg_trycallback_fromisr()
 *   (for FreeRTOS, mainly)
 * * socket API: support poll(), sendmsg() and recvmsg(); fix problems on close
 * 
 * Detailed Changelog
 * ------------------
 * @verbinclude "CHANGELOG"
 */

/**
 * @page contrib How to contribute to lwIP
 * @verbinclude "contrib.txt"
 */

/**
 * @page pitfalls Common pitfalls
 *
 * Multiple Execution Contexts in lwIP code
 * ========================================
 *
 * The most common source of lwIP problems is to have multiple execution contexts
 * inside the lwIP code.
 * 
 * lwIP can be used in two basic modes: @ref lwip_nosys (no OS/RTOS 
 * running on target system) or @ref lwip_os (there is an OS running
 * on the target system).
 * 
 * See also: @ref multithreading (especially the part about @ref LWIP_ASSERT_CORE_LOCKED()!)
 *
 * Mainloop Mode
 * -------------
 * In mainloop mode, only @ref callbackstyle_api can be used.
 * The user has two possibilities to ensure there is only one 
 * exection context at a time in lwIP:
 *
 * 1) Deliver RX ethernet packets directly in interrupt context to lwIP
 *    by calling netif->input directly in interrupt. This implies all lwIP 
 *    callback functions are called in IRQ context, which may cause further
 *    problems in application code: IRQ is blocked for a long time, multiple
 *    execution contexts in application code etc. When the application wants
 *    to call lwIP, it only needs to disable interrupts during the call.
 *    If timers are involved, even more locking code is needed to lock out
 *    timer IRQ and ethernet IRQ from each other, assuming these may be nested.
 *
 * 2) Run lwIP in a mainloop. There is example code here: @ref lwip_nosys.
 *    lwIP is _ONLY_ called from mainloop callstacks here. The ethernet IRQ
 *    has to put received telegrams into a queue which is polled in the
 *    mainloop. Ensure lwIP is _NEVER_ called from an interrupt, e.g.
 *    some SPI IRQ wants to forward data to udp_send() or tcp_write()!
 *
 * OS Mode
 * -------
 * In OS mode, @ref callbackstyle_api AND @ref sequential_api can be used.
 * @ref sequential_api are designed to be called from threads other than
 * the TCPIP thread, so there is nothing to consider here.
 * But @ref callbackstyle_api functions must _ONLY_ be called from
 * TCPIP thread. It is a common error to call these from other threads
 * or from IRQ contexts. ​Ethernet RX needs to deliver incoming packets
 * in the correct way by sending a message to TCPIP thread, this is
 * implemented in tcpip_input().​​
 * Again, ensure lwIP is _NEVER_ called from an interrupt, e.g.
 * some SPI IRQ wants to forward data to udp_send() or tcp_write()!
 * 
 * 1) tcpip_callback() can be used get called back from TCPIP thread,
 *    it is safe to call any @ref callbackstyle_api from there.
 *
 * 2) Use @ref LWIP_TCPIP_CORE_LOCKING. All @ref callbackstyle_api
 *    functions can be called when lwIP core lock is aquired, see
 *    @ref LOCK_TCPIP_CORE() and @ref UNLOCK_TCPIP_CORE().
 *    These macros cannot be used in an interrupt context!
 *    Note the OS must correctly handle priority inversion for this.
 *
 * Cache / DMA issues
 * ==================
 *
 * DMA-capable ethernet hardware and zero-copy RX
 * ----------------------------------------------
 * 
 * lwIP changes the content of RECEIVED pbufs in the TCP code path.
 * This implies one or more cacheline(s) of the RX pbuf become dirty
 * and need to be flushed before the memory is handed over to the
 * DMA ethernet hardware for the next telegram to be received.
 * See http://lists.nongnu.org/archive/html/lwip-devel/2017-12/msg00070.html
 * for a more detailed explanation.
 * Also keep in mind the user application may also write into pbufs,
 * so it is generally a bug not to flush the data cache before handing
 * a buffer to DMA hardware.
 *
 * DMA-capable ethernet hardware and cacheline alignment
 * -----------------------------------------------------
 * Nice description about DMA capable hardware and buffer handling:
 * http://www.pebblebay.com/a-guide-to-using-direct-memory-access-in-embedded-systems-part-two/
 * Read especially sections "Cache coherency" and "Buffer alignment".
 */

/**
 * @page bugs Reporting bugs
 * Please report bugs in the lwIP bug tracker at savannah.\n
 * BEFORE submitting, please check if the bug has already been reported!\n
 * https://savannah.nongnu.org/bugs/?group=lwip
 */

/**
 * @page zerocopyrx Zero-copy RX
 * The following code is an example for zero-copy RX ethernet driver:
 * @include ZeroCopyRx.c
 */

/**
 * @defgroup lwip_nosys Mainloop mode ("NO_SYS")
 * @ingroup lwip
 * Use this mode if you do not run an OS on your system. \#define NO_SYS to 1.
 * Feed incoming packets to netif->input(pbuf, netif) function from mainloop,
 * *not* *from* *interrupt* *context*. You can allocate a @ref pbuf in interrupt
 * context and put them into a queue which is processed from mainloop.\n
 * Call sys_check_timeouts() periodically in the mainloop.\n
 * Porting: implement all functions in @ref sys_time, @ref sys_prot and 
 * @ref compiler_abstraction.\n
 * You can only use @ref callbackstyle_api in this mode.\n
 * Sample code:\n
 * @include NO_SYS_SampleCode.c
 */

/**
 * @defgroup lwip_os OS mode (TCPIP thread)
 * @ingroup lwip
 * Use this mode if you run an OS on your system. It is recommended to
 * use an RTOS that correctly handles priority inversion and
 * to use @ref LWIP_TCPIP_CORE_LOCKING.\n
 * Porting: implement all functions in @ref sys_layer.\n
 * You can use @ref callbackstyle_api together with @ref tcpip_callback,
 * and all @ref sequential_api.
 */

/**
 * @page sys_init System initalization
A truly complete and generic sequence for initializing the lwIP stack
cannot be given because it depends on additional initializations for
your runtime environment (e.g. timers).

We can give you some idea on how to proceed when using the raw API.
We assume a configuration using a single Ethernet netif and the
UDP and TCP transport layers, IPv4 and the DHCP client.

Call these functions in the order of appearance:

- lwip_init(): Initialize the lwIP stack and all of its subsystems.

- netif_add(struct netif *netif, ...):
  Adds your network interface to the netif_list. Allocate a struct
  netif and pass a pointer to this structure as the first argument.
  Give pointers to cleared ip_addr structures when using DHCP,
  or fill them with sane numbers otherwise. The state pointer may be NULL.

  The init function pointer must point to a initialization function for
  your Ethernet netif interface. The following code illustrates its use.
  
@code{.c}
  err_t netif_if_init(struct netif *netif)
  {
    u8_t i;
    
    for (i = 0; i < ETHARP_HWADDR_LEN; i++) {
      netif->hwaddr[i] = some_eth_addr[i];
    }
    init_my_eth_device();
    return ERR_OK;
  }
@endcode
  
  For Ethernet drivers, the input function pointer must point to the lwIP
  function ethernet_input() declared in "netif/etharp.h". Other drivers
  must use ip_input() declared in "lwip/ip.h".
  
- netif_set_default(struct netif *netif)
  Registers the default network interface.

- netif_set_link_up(struct netif *netif)
  This is the hardware link state; e.g. whether cable is plugged for wired
  Ethernet interface. This function must be called even if you don't know
  the current state. Having link up and link down events is optional but
  DHCP and IPv6 discover benefit well from those events.

- netif_set_up(struct netif *netif)
  This is the administrative (= software) state of the netif, when the
  netif is fully configured this function must be called.

- dhcp_start(struct netif *netif)
  Creates a new DHCP client for this interface on the first call.
  You can peek in the netif->dhcp struct for the actual DHCP status.

- sys_check_timeouts()
  When the system is running, you have to periodically call
  sys_check_timeouts() which will handle all timers for all protocols in
  the stack; add this to your main loop or equivalent.
 */

/**
 * @page multithreading Multithreading
 * lwIP started targeting single-threaded environments. When adding multi-
 * threading support, instead of making the core thread-safe, another
 * approach was chosen: there is one main thread running the lwIP core
 * (also known as the "tcpip_thread"). When running in a multithreaded
 * environment, raw API functions MUST only be called from the core thread
 * since raw API functions are not protected from concurrent access (aside
 * from pbuf- and memory management functions). Application threads using
 * the sequential- or socket API communicate with this main thread through
 * message passing.
 * 
 * As such, the list of functions that may be called from
 * other threads or an ISR is very limited! Only functions
 * from these API header files are thread-safe:
 * - api.h
 * - netbuf.h
 * - netdb.h
 * - netifapi.h
 * - pppapi.h
 * - sockets.h
 * - sys.h
 * 
 * Additionaly, memory (de-)allocation functions may be
 * called from multiple threads (not ISR!) with NO_SYS=0
 * since they are protected by @ref SYS_LIGHTWEIGHT_PROT and/or
 * semaphores.
 * 
 * Netconn or Socket API functions are thread safe against the
 * core thread but they are not reentrant at the control block
 * granularity level. That is, a UDP or TCP control block must
 * not be shared among multiple threads without proper locking.
 * 
 * If @ref SYS_LIGHTWEIGHT_PROT is set to 1 and
 * @ref LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT is set to 1,
 * pbuf_free() may also be called from another thread or
 * an ISR (since only then, mem_free - for PBUF_RAM - may
 * be called from an ISR: otherwise, the HEAP is only
 * protected by semaphores).
 * 
 * How to get threading done right
 * -------------------------------
 * 
 * It is strongly recommended to implement the LWIP_ASSERT_CORE_LOCKED()
 * macro in an application that uses multithreading. lwIP code has
 * several places where a check for a correct thread context is
 * implemented which greatly helps the user to get threading done right.
 * See the example sys_arch.c files in unix and Win32 port 
 * in the contrib repository.
 * 
 * In short: Copy the functions sys_mark_tcpip_thread() and 
 * sys_check_core_locking() to your port and modify them to work with your OS.
 * Then let @ref LWIP_ASSERT_CORE_LOCKED() and @ref LWIP_MARK_TCPIP_THREAD()
 * point to these functions.
 * 
 * If you use @ref LWIP_TCPIP_CORE_LOCKING, you also need to copy and adapt
 * the functions sys_lock_tcpip_core() and sys_unlock_tcpip_core().
 * Let @ref LOCK_TCPIP_CORE() and @ref UNLOCK_TCPIP_CORE() point 
 * to these functions. 
 */

/**
 * @page optimization Optimization hints
The first thing you want to optimize is the lwip_standard_checksum()
routine from src/core/inet.c. You can override this standard
function with the \#define LWIP_CHKSUM your_checksum_routine().

There are C examples given in inet.c or you might want to
craft an assembly function for this. RFC1071 is a good
introduction to this subject.

Other significant improvements can be made by supplying
assembly or inline replacements for htons() and htonl()
if you're using a little-endian architecture.
\#define lwip_htons(x) your_htons()
\#define lwip_htonl(x) your_htonl()
If you \#define them to htons() and htonl(), you should
\#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS to prevent lwIP from
defining htonx / ntohx compatibility macros.

Check your network interface driver if it reads at
a higher speed than the maximum wire-speed. If the
hardware isn't serviced frequently and fast enough
buffer overflows are likely to occur.

E.g. when using the cs8900 driver, call cs8900if_service(ethif)
as frequently as possible. When using an RTOS let the cs8900 interrupt
wake a high priority task that services your driver using a binary
semaphore or event flag. Some drivers might allow additional tuning
to match your application and network.

For a production release it is recommended to set LWIP_STATS to 0.
Note that speed performance isn't influenced much by simply setting
high values to the memory options.
 */
