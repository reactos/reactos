#ifndef FIFO_H
#define FIFO_H

#include "lwip/sys.h"

/** How many bytes in fifo */
#define FIFOSIZE 2048

/** fifo data structure, this one is passed to all fifo functions */
typedef struct fifo_t {
  u8_t data[FIFOSIZE+10]; /* data segment, +10 is a hack probably not needed.. FIXME! */
  int dataslot;			  /* index to next char to be read */
  int emptyslot;		  /* index to next empty slot */
  int len;				  /* len probably not needed, may be calculated from dataslot and emptyslot in conjunction with FIFOSIZE */

  sys_sem_t sem;		/* semaphore protecting simultaneous data manipulation */
  sys_sem_t getSem;		/* sepaphore used to signal new data if getWaiting is set */
  u8_t getWaiting;		/* flag used to indicate that fifoget is waiting for data. fifoput is supposed to clear */
  						/* this flag prior to signaling the getSem semaphore */
} fifo_t;


/**
*   Get a character from fifo
*   Blocking call.
*	@param 	fifo pointer to fifo data structure
*	@return	character read from fifo
*/
u8_t fifoGet(fifo_t * fifo);

/**
*   Get a character from fifo
*   Non blocking call.
*	@param 	fifo pointer to fifo data structure
*	@return	character read from fifo, or < zero if non was available
*/
s16_t fifoGetNonBlock(fifo_t * fifo);

/**
*	fifoput is called by the signalhandler when new data has arrived (or some other event is indicated)
*   fifoput reads directly from the serialport and is thus highly dependent on unix arch at this moment
*	@param 	fifo pointer to fifo data structure
*	@param	fd	unix file descriptor
*/
void fifoPut(fifo_t * fifo, int fd);

/**
*   fifoinit initiate fifo
*	@param 	fifo	pointer to fifo data structure, allocated by the user
*/
void fifoInit(fifo_t * fifo);

#endif

