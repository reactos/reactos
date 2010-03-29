#ifndef __UNIATA_CONFIG__H__
#define __UNIATA_CONFIG__H__


/***************************************************/
/*        Options                                  */
/***************************************************/

/***************************************/
// Send Debug messages directly to DbgPrintLonner using its SDK
/***************************************/

//#define USE_DBGPRINT_LOGGER

/***************************************/
// Send Debug messages via ScsiPort API
/***************************************/

//#define SCSI_PORT_DBG_PRINT

/***************************************/
// Using DbgPrint on raised IRQL will crash w2k
// this will not happen immediately, so we shall see some logs.
// You can tune Irql checking here
// Note: you can avoid crashes if configure DbgPrintLogger to check Irql
/***************************************/

#define LOG_ON_RAISED_IRQL_W2K    TRUE
//#define LOG_ON_RAISED_IRQL_W2K    FALSE

/***************************************/
// Use hack to avoid PCI-ISA DMA limitations (physical memory must
// be allocated below 16Mb). Actually there is no such limitation,
// so we have to pretent to be PIO and converl logical addresses
// to physical manually
/***************************************/

#define USE_OWN_DMA

/***************************************/
// Special option, enables dumping of ATAPI cammands and data buffers
// via DbgPrint API
/***************************************/

//#define UNIATA_DUMP_ATAPI

/***************************************/
// Optimization for uni-processor machines
/***************************************/

//#define UNI_CPU_OPTIMIZATION

/***************************************/
// Enable/disable performance statistics
/***************************************/

#define QUEUE_STATISTICS

#define IO_STATISTICS

/***************************************/
// Misc
/***************************************/

//#define NAVO_TEST

/***************************************************/
/*    Validate Options                             */
/***************************************************/

#ifdef _DEBUG

 #ifndef DBG
  #define DBG
 #endif //DBG

#else //_DEBUG

 #ifdef USE_DBGPRINT_LOGGER
  #undef USE_DBGPRINT_LOGGER
 #endif //USE_DBGPRINT_LOGGER

#endif // !_DEBUG

/***************************************************/
/*  Compiler dependencies                          */
/***************************************************/

#define DDKFASTAPI      __fastcall

/* Are we under GNU C (mingw) ??? */
#ifdef __GNUC__

 #define  DEF_U64(x)     (x##ULL)
 #define  DEF_I64(x)     (x##LL)

#else // !__GNUC__ => MSVC/Intel

 #define  DEF_U64(x)     (x##UI64)
 #define  DEF_I64(x)     (x##I64)

#endif //__GNUC__


#endif //__UNIATA_CONFIG__H__
