/* queue.h */

/*********************************/
/* Definitions                   */
/*********************************/

#define		QUEUE_MAGIC	0xBADD



/*********************************/
/* Structure Definitions         */
/*********************************/

typedef struct {
	void		*next;				// Next element
	void		*prev;				// Previous element
	void		*data;				// Data
} Element_t;

typedef struct {
	Element_t	*elem;				// Next element to be returned
} Iter_t;

typedef struct {
	int			magic;				// Magic number
	Element_t	*head;				// Head of queue
	Element_t	*tail;				// End of queue
	long		count;				// Number of entries
	long		mark;				// Watermark
} Queue_t;


/*********************************/
/* Function Definitions          */
/*********************************/
Element_t *elementAlloc();
void elementFree(Element_t *element);
void queueInit (Queue_t *queue);
Element_t *queueAdd (Queue_t *queue, Element_t *element);
Element_t *queueRemove (Queue_t *queue, Element_t *element);
void queueFree (Queue_t *queue);
void iterInit (Queue_t *queue, Iter_t *iter);
Element_t *queueIterate (Iter_t *iter);

