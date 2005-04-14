/*
 * Shared producer-consumer ring macros.
 * Tim Deegan and Andrew Warfield November 2004.
 */ 

#ifndef __XEN_PUBLIC_IO_RING_H__
#define __XEN_PUBLIC_IO_RING_H__

typedef unsigned int RING_IDX;

/* Round a 32-bit unsigned constant down to the nearest power of two. */
#define __RD2(_x)  (((_x) & 0x00000002) ? 0x2                  : ((_x) & 0x1))
#define __RD4(_x)  (((_x) & 0x0000000c) ? __RD2((_x)>>2)<<2    : __RD2(_x))
#define __RD8(_x)  (((_x) & 0x000000f0) ? __RD4((_x)>>4)<<4    : __RD4(_x))
#define __RD16(_x) (((_x) & 0x0000ff00) ? __RD8((_x)>>8)<<8    : __RD8(_x))
#define __RD32(_x) (((_x) & 0xffff0000) ? __RD16((_x)>>16)<<16 : __RD16(_x))

/*
 * Calculate size of a shared ring, given the total available space for the
 * ring and indexes (_sz), and the name tag of the request/response structure.
 * A ring contains as many entries as will fit, rounded down to the nearest 
 * power of two (so we can mask with (size-1) to loop around).
 */
#define __RING_SIZE(_s, _sz) \
    (__RD32(((_sz) - 2*sizeof(RING_IDX)) / sizeof((_s)->ring[0])))

/*
 *  Macros to make the correct C datatypes for a new kind of ring.
 * 
 *  To make a new ring datatype, you need to have two message structures,
 *  let's say request_t, and response_t already defined.
 *
 *  In a header where you want the ring datatype declared, you then do:
 *
 *     DEFINE_RING_TYPES(mytag, request_t, response_t);
 *
 *  These expand out to give you a set of types, as you can see below.
 *  The most important of these are:
 *  
 *     mytag_sring_t      - The shared ring.
 *     mytag_front_ring_t - The 'front' half of the ring.
 *     mytag_back_ring_t  - The 'back' half of the ring.
 *
 *  To initialize a ring in your code you need to know the location and size
 *  of the shared memory area (PAGE_SIZE, for instance). To initialise
 *  the front half:
 *
 *      mytag_front_ring_t front_ring;
 *
 *      SHARED_RING_INIT((mytag_sring_t *)shared_page);
 *      FRONT_RING_INIT(&front_ring, (mytag_sring_t *)shared_page, PAGE_SIZE);
 *
 *  Initializing the back follows similarly...
 */
         
#define DEFINE_RING_TYPES(__name, __req_t, __rsp_t)                     \
                                                                        \
/* Shared ring entry */                                                 \
union __name##_sring_entry {                                            \
    __req_t req;                                                        \
    __rsp_t rsp;                                                        \
};                                                                      \
                                                                        \
/* Shared ring page */                                                  \
struct __name##_sring {                                                 \
    RING_IDX req_prod;                                                  \
    RING_IDX rsp_prod;                                                  \
    union __name##_sring_entry ring[1]; /* variable-length */           \
};                                                                      \
                                                                        \
/* "Front" end's private variables */                                   \
struct __name##_front_ring {                                            \
    RING_IDX req_prod_pvt;                                              \
    RING_IDX rsp_cons;                                                  \
    unsigned int nr_ents;                                               \
    struct __name##_sring *sring;                                       \
};                                                                      \
                                                                        \
/* "Back" end's private variables */                                    \
struct __name##_back_ring {                                             \
    RING_IDX rsp_prod_pvt;                                              \
    RING_IDX req_cons;                                                  \
    unsigned int nr_ents;                                               \
    struct __name##_sring *sring;                                       \
};                                                                      \
                                                                        \
/* Syntactic sugar */                                                   \
typedef struct __name##_sring __name##_sring_t;                         \
typedef struct __name##_front_ring __name##_front_ring_t;               \
typedef struct __name##_back_ring __name##_back_ring_t;

/*
 *   Macros for manipulating rings.  
 * 
 *   FRONT_RING_whatever works on the "front end" of a ring: here 
 *   requests are pushed on to the ring and responses taken off it.
 * 
 *   BACK_RING_whatever works on the "back end" of a ring: here 
 *   requests are taken off the ring and responses put on.
 * 
 *   N.B. these macros do NO INTERLOCKS OR FLOW CONTROL.  
 *   This is OK in 1-for-1 request-response situations where the 
 *   requestor (front end) never has more than RING_SIZE()-1
 *   outstanding requests.
 */

/* Initialising empty rings */
#define SHARED_RING_INIT(_s) do {                                       \
    (_s)->req_prod = 0;                                                 \
    (_s)->rsp_prod = 0;                                                 \
} while(0)

#define FRONT_RING_INIT(_r, _s, __size) do {                            \
    (_r)->req_prod_pvt = 0;                                             \
    (_r)->rsp_cons = 0;                                                 \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
    (_r)->sring = (_s);                                                 \
} while (0)

#define BACK_RING_INIT(_r, _s, __size) do {                             \
    (_r)->rsp_prod_pvt = 0;                                             \
    (_r)->req_cons = 0;                                                 \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
    (_r)->sring = (_s);                                                 \
} while (0)

/* Initialize to existing shared indexes -- for recovery */
#define FRONT_RING_ATTACH(_r, _s, __size) do {                          \
    (_r)->sring = (_s);                                                 \
    (_r)->req_prod_pvt = (_s)->req_prod;                                \
    (_r)->rsp_cons = (_s)->rsp_prod;                                    \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
} while (0)

#define BACK_RING_ATTACH(_r, _s, __size) do {                           \
    (_r)->sring = (_s);                                                 \
    (_r)->rsp_prod_pvt = (_s)->rsp_prod;                                \
    (_r)->req_cons = (_s)->req_prod;                                    \
    (_r)->nr_ents = __RING_SIZE(_s, __size);                            \
} while (0)

/* How big is this ring? */
#define RING_SIZE(_r)                                                   \
    ((_r)->nr_ents)

/* How many empty slots are on a ring? */
#define RING_PENDING_REQUESTS(_r)                                       \
   ( ((_r)->req_prod_pvt - (_r)->rsp_cons) )
   
/* Test if there is an empty slot available on the front ring. 
 * (This is only meaningful from the front. )
 */
#define RING_FULL(_r)                                                   \
    (((_r)->req_prod_pvt - (_r)->rsp_cons) == RING_SIZE(_r))

/* Test if there are outstanding messages to be processed on a ring. */
#define RING_HAS_UNCONSUMED_RESPONSES(_r)                               \
   ( (_r)->rsp_cons != (_r)->sring->rsp_prod )
   
#define RING_HAS_UNCONSUMED_REQUESTS(_r)                                \
   ( ((_r)->req_cons != (_r)->sring->req_prod ) &&                      \
     (((_r)->req_cons - (_r)->rsp_prod_pvt) !=                          \
      RING_SIZE(_r)) )
      
/* Test if there are messages waiting to be pushed. */
#define RING_HAS_UNPUSHED_REQUESTS(_r)                                  \
   ( (_r)->req_prod_pvt != (_r)->sring->req_prod )
   
#define RING_HAS_UNPUSHED_RESPONSES(_r)                                 \
   ( (_r)->rsp_prod_pvt != (_r)->sring->rsp_prod )

/* Copy the private producer pointer into the shared ring so the other end 
 * can see the updates we've made. */
#define RING_PUSH_REQUESTS(_r) do {                                     \
    wmb();                                                              \
    (_r)->sring->req_prod = (_r)->req_prod_pvt;                         \
} while (0)

#define RING_PUSH_RESPONSES(_r) do {                                    \
    wmb();                                                              \
    (_r)->sring->rsp_prod = (_r)->rsp_prod_pvt;                         \
} while (0)

/* Direct access to individual ring elements, by index. */
#define RING_GET_REQUEST(_r, _idx)                                      \
 (&((_r)->sring->ring[                                                  \
     ((_idx) & (RING_SIZE(_r) - 1))                                     \
     ].req))

#define RING_GET_RESPONSE(_r, _idx)                                     \
 (&((_r)->sring->ring[                                                  \
     ((_idx) & (RING_SIZE(_r) - 1))                                     \
     ].rsp))   
    
/* Loop termination condition: Would the specified index overflow the ring? */
#define RING_REQUEST_CONS_OVERFLOW(_r, _cons)                           \
    (((_cons) - (_r)->rsp_prod_pvt) >= RING_SIZE(_r))

#endif /* __XEN_PUBLIC_IO_RING_H__ */
