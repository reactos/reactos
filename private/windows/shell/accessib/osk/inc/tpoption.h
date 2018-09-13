//#pragma once
#include <time.h>
#define MAX_HISTORY	10
#define STATVERSION 249 

/*typedef struct Statistics	{
	short	version;
	short	index;
	time_t	begTime[MAX_HISTORY];
	time_t	endTime[MAX_HISTORY];
	short	realKeys[MAX_HISTORY];
	short	postKeys[MAX_HISTORY];
	short	newDicts[MAX_HISTORY];
	short	newWords[MAX_HISTORY];
	short	newNexts[MAX_HISTORY];
	short	hitWords[MAX_HISTORY];
	short	hitAbbrs[MAX_HISTORY];
	short	mouseDowns[MAX_HISTORY];
	short	switches[MAX_HISTORY];
	} Statistics;
*/

typedef struct TSDict {
//	short	version;	
// structure version
	char	name[32];	 // name of this topic 

    //flags
//	unsigned DICT_LRNNEW  : 1;	// learn new words
//	unsigned DICT_LRNNEXT : 1;	// learn next words
//	unsigned DICT_LRNFREQ : 1;	// adjust word freq
//	unsigned DICT_SYMBOL  : 1;	// Symbolic dictionary (ie: Minspeak)
//	unsigned DICT_2BYTE   : 1;	// Two byte symbols (ie: Kanji)
//	unsigned DICT_REVERSE : 1;	// Runs right to left (ie: Hebrew)
//	unsigned DICT_PREDICT : 1;	// Use for predictions
//	unsigned DICT_PURGAUTO: 1;	// Autopurge if too full?
//	unsigned DICT_PURGASK : 1;	// Ask before purging
//	unsigned DICT_CURRENT : 1;	// current option item
//	unsigned DICT_LRNNUM  : 1;  // learn number
//	unsigned CAP		  : 1;  // capitialize new sentence

//	short	maxWords;	// words per topic
//	short	minLength;	// minimum word length
//	short	weight;		// how much to weight the probs
//	short   space;      //how many space after sentence

	short	tnmbr[256];	// how many in chain
	struct TSFreq *atops[256];	// first word in alpha chain
	struct TSFreq *ftops[256];	// first word in freq chain

//	struct TSFreq *dtops[256];	// first word in disamb chain
//	short	indx;		        // TSFreq Group selected
//	short	nmbr;		        // how many in chain?
	struct TSFreq  *last;	    // last word seen
//	struct TSFreq  *nearest;	// word nearest to word to insert
	}	TSDict;

extern struct TSDict *dp;

typedef struct TSFreq
	{
	// flags
//	unsigned WORD_SLCT   	:1;	// this word was selected in last match
	unsigned WORD_LOCK   	:1; // never delete word
//	unsigned WORD_CASE   	:1;	// word is case sensitive
	unsigned WORD_ABBR   	:1;	// word is an abbreviation
//	unsigned WORD_IGNR   	:1;	// ignore this word
//	unsigned WORD_SAY    	:1;	// say this word
	unsigned WORD_TYPE   	:1;	// type this word
//	unsigned WORD_NNXT   	:1;	// don't learn next words
//	unsigned WORD_SCRIPT 	:1;	// word data is a script
//	unsigned WORD_FPROB		:1;	// don't change freq
//	unsigned WORD_PREV		:1;	// this word was previously shown
//	unsigned WORD_HELD		:1;	// this word was held over for next
//	unsigned WORD_DELETED 	:1;	// this word was deleted
//	unsigned WORD_BIAS		:1;	// this is a preferred word in this session
//	unsigned WORD_VOICE		:1;	// the word/abbr special pronounciation

	short	 freq;	//	freq for this word

	struct TSNext	*nhead;	//	head of next word list for this word
	struct TSFreq	*anext;	//	next entry alphabetical
	struct TSFreq	*fnext;	//	next entry frequency
//	struct TSFreq	*dnext;	//	next entry disambiguation
	char 	 	*word;	//	actual data
	char		*abbr;	//	actual data
	} TSFreq;

extern struct TSFreq  *wordCur,  *abbrCur;

typedef struct TSNext
	{
	short	freq;	// this word pair's freq
	struct TSFreq	*nword;	// next word
	struct TSNext	*nnext;	// next word pair in chain
	} TSNext;

//extern void *MemPool;
//extern BYTE *MemPoolPtr;   

