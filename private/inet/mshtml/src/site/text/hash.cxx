/*
 *  @doc    INTERNAL
 *
 *  @module HASH.C -- RTF control word cache |
 *      #ifdef'ed with RTF_HASHCACHE
 *      
 *  Owner: <nl>
 *      Jon Matousek <nl>
 *
 *  History: <nl>
 *      8/15/95     jonmat first hash-cache for RTF using Brent's Method.
 */

#include "headers.hxx"

#ifdef RTF_HASHCACHE
                     
#ifndef X_HASH_H_
#define X_HASH_H_
#include "hash.h"
#endif

extern KEYWORD      rgKeyword[];            // All of the RTF control words.

#define MAX_INAME   3

typedef struct {
    const KEYWORD   *token;
    BOOL            passBit;
} HashEntry;

static HashEntry    *(hashtbl[HASHSIZE]);
static HashEntry    *storage;               // Dynamically alloc for cKeywords.

BOOL                _rtfHashInited = FALSE;

static INT          HashKeyword_Key( const CHAR *szKeyword );

/*
 *  HashKeyword_Insert()
 *  
 *  @func
 *      Insert a KEYWORD into the RTF hash table.
 *  @comm
 *      This function uses the the % for MOD
 *      in order to validate MOD257.
 */
VOID HashKeyword_Insert (
    const KEYWORD *token )//@parm pointer to KEYWORD token to insert.
{
    INT         index, step, position,
                cost, source, sink, index1,
                step1, temp;
    
    BOOL        tmpPassBit; 
    
    static INT  totalKeys = 0;

    CHAR        *szKeyword;
    
    HashEntry   *np;
    
    AssertSz ( _rtfHashInited, "forgot to call HashKeyword_Init()");
    AssertSz ( totalKeys <= HASHSIZE * 0.7, "prime not large enough to hold total keys");
    
    szKeyword = token->szKeyword;
    
    np = &storage[totalKeys++];
    np->token = token;

    index = HashKeyword_Key(szKeyword) % HASHSIZE;  // Get keys.
    step = 1 + (HashKeyword_Key(szKeyword) % (HASHSIZE-1));

    position = 1;
    cost = HASHSIZE;                                // The max collisions for any.
    while(hashtbl[index]!=NULL)                     // Find empty slot.
    {
        position++;                                 // How many collisions.

        // For the keyword stored here, calc # times before it is found.
        temp=1;
        step1= 1+(HashKeyword_Key(hashtbl[index]->token->szKeyword) % (HASHSIZE-1));
        index1= (index+step1)%HASHSIZE;
        while(hashtbl[index1] !=NULL)
        {
            index1=(index1+step1)%HASHSIZE;
            temp++;
        }
        
        // Incremental cost computation, minimizes average # of collisions
        //  for both keywords.
        if (cost>position+temp)
        {
            source=index;
            sink=index1;
            cost=position+temp;
        }
        
        // There will be something stored beyound here, set the passBit.
        hashtbl[index]->passBit=1;

        // Next index to search for empty slot.
        index=(index+step)%HASHSIZE;

    }
    
    if (position<=cost)
    {
        source=sink=index;
        cost=position;
    }
    hashtbl[sink] = hashtbl[source];
    hashtbl[source] = np;
    if (hashtbl[sink] && hashtbl[source])   // jOn hack, we didn't really
    {                                       //  want to swap pass bits.
        tmpPassBit = hashtbl[sink]->passBit;
        hashtbl[sink]->passBit = hashtbl[source]->passBit;
        hashtbl[source]->passBit = tmpPassBit;
    }

}

/*
 *  static HashKeyword_Key()
 *  
 *  @func
 *      Calculate the hash key.
 *  @comm
 *      Just add up the first few characters.
 *  @rdesc
 *      The hash Key for calculating the index and step.
 */
static INT HashKeyword_Key(
    const CHAR *szKeyword ) //@parm C string to create hash key for.
{
    INT i, tot = 0;
    
    /* Just add up first few characters. */
    for (i = 0; i < MAX_INAME && *szKeyword; szKeyword++, i++)
            tot += (UCHAR) *szKeyword;
    return tot;
}   

/*
 *  HashKeyword_Fetch()
 *  
 *  @func
 *      Look up a KEYWORD with the given szKeyword.
 *  @devnote
 *      We have a hash table of size 257. This allows for
 *      the use of very fast routines to calculate a MOD 257.
 *      This gives us a significant increase in performance
 *      over a binary search.
 *  @rdesc
 *      A pointer to the KEYWORD, or NULL if not found.
 */
const KEYWORD *HashKeyword_Fetch (
    const CHAR *szKeyword ) //@parm C string to search for.
{
    INT         index, step;
    
    HashEntry * hashTblPtr;

    BYTE *      pchCandidate;
    BYTE *      pchKeyword;
    
    INT         nComp;

    CHAR        firstChar;

    INT         hashKey;

    AssertSz( HASHSIZE == 257, "Remove custom MOD257.");
    
    firstChar = *szKeyword;
    hashKey = HashKeyword_Key(szKeyword);   // For calc'ing 'index' and 'step'
    
    //index = hashKey%HASHSIZE;             // First entry to search.
    index = MOD257(hashKey);                // This formula gives us 18% perf.

    hashTblPtr = hashtbl[index];            // Get first entry.
    if ( hashTblPtr != NULL )               // Something there?
    {
                                            // Compare 2 C strings.                             
        pchCandidate = (BYTE *)hashTblPtr->token->szKeyword;
        if ( firstChar == *pchCandidate )
        {
            pchKeyword   = (BYTE *)szKeyword;
            while (!(nComp = *pchKeyword - *pchCandidate)   // Be sure to match
                && *pchKeyword)                             //  terminating 0's
            {
                pchKeyword++;
                pchCandidate++;
            }
                                            // Matched?
            if ( 0 == nComp )
                return hashTblPtr->token;
        }
        
        if ( hashTblPtr->passBit==1 )       // passBit=>another entry to test
        {

            // step = 1+(hashKey%(HASHSIZE-1));// Calc 'step'
            step = 1 + MOD257_1(hashKey);

                                            // Get second entry to check.
            index += step;
            index = MOD257(index);
            hashTblPtr = hashtbl[index];

            while (hashTblPtr != NULL )     // While something there.
            {
                                            // Compare 2 C strings.                             
                pchCandidate = (BYTE *)hashTblPtr->token->szKeyword;
                if ( firstChar == *pchCandidate )
                {
                    pchKeyword   = (BYTE *)szKeyword;
                    while (!(nComp = *pchKeyword - *pchCandidate)
                        && *pchKeyword)
                    {
                        pchKeyword++;
                        pchCandidate++;
                    }
                                            // Matched?
                    if ( 0 == nComp )
                        return hashTblPtr->token;
                }

                if ( !hashTblPtr->passBit )// Done searching?
                    break;
                                            // Get next entry.
                index += step;
                index = MOD257(index);
                hashTblPtr = hashtbl[index];
            }
        }
    }
    
    return NULL;
}

/*
 *  HashKeyword_Init()
 *  
 *  @func
 *      Load up and init the hash table with RTF control words.
 *  @devnote
 *      _rtfHashInited will be FALSE if anything here fails.
 */
VOID HashKeyword_Init( )
{
    extern SHORT cKeywords;         // How many RTF keywords we currently recognize.

    INT i;

    AssertSz( _rtfHashInited == FALSE, "Only need to init this once.");

                                    // Create enough storage for cKeywords
    storage = (HashEntry *) MemAllocClear( sizeof(HashEntry) * cKeywords );

                                    // Load in all of the RTF control words.
    if ( storage )
    {
        _rtfHashInited = TRUE;

        for (i = 0; i < cKeywords; i++ )
        {
            HashKeyword_Insert(&rgKeyword[i]);
        }
#if DBG==1                        // Make sure we can fetch all these keywords.
        for (i = 0; i < cKeywords; i++ )
        {
            AssertSz ( &rgKeyword[i] == HashKeyword_Fetch ( rgKeyword[i].szKeyword ),
                "Keyword Hash is not working.");
        }
#endif
    }
}

#endif  // RTF_HASHCACHE
