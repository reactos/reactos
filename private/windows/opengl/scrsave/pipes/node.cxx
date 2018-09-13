/******************************Module*Header*******************************\
* Module Name: node.cxx
*
* Pipes node array
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>
#include <windows.h>

#include "sspipes.h"
#include "node.h"


/**************************************************************************\
*
* NODE_ARRAY constructor
*
\**************************************************************************/

NODE_ARRAY::NODE_ARRAY()
{
    nodes = NULL; // allocated on Resize

    numNodes.x = 0;
    numNodes.y = 0;
    numNodes.z = 0;
}

/**************************************************************************\
*
* NODE_ARRAY destructor
*
\**************************************************************************/

NODE_ARRAY::~NODE_ARRAY( )
{
    if( nodes )
        delete nodes;
}

/**************************************************************************\
*
* Resize
* 
\**************************************************************************/ 

void
NODE_ARRAY::Resize( IPOINT3D *pNewSize )
{
    if( (numNodes.x == pNewSize->x) &&
        (numNodes.y == pNewSize->y) &&
        (numNodes.z == pNewSize->z) )
        return;

    numNodes = *pNewSize;

    int elemCount = numNodes.x * numNodes.y * numNodes.z ;

    if( nodes )
        delete nodes;

    nodes = new Node[elemCount];

    SS_ASSERT( nodes, "NODE_ARRAY::Resize : can't alloc nodes\n" );

    // Reset the node states to empty

    int i;
    Node *pNode = nodes;
    for( i = 0; i < elemCount; i++, pNode++ )
        pNode->MarkAsEmpty();

    // precalculate direction offsets between nodes for speed
    nodeDirInc[PLUS_X] = 1;
    nodeDirInc[MINUS_X] = -1;
    nodeDirInc[PLUS_Y] = numNodes.x;
    nodeDirInc[MINUS_Y] = - nodeDirInc[PLUS_Y];
    nodeDirInc[PLUS_Z] = numNodes.x * numNodes.y;
    nodeDirInc[MINUS_Z] = - nodeDirInc[PLUS_Z];
}

/**************************************************************************\
*
* Reset
*
\**************************************************************************/

void
NODE_ARRAY::Reset( )
{
    int i;
    Node *pNode = nodes;

    // Reset the node states to empty
    for( i = 0; i < (numNodes.x)*(numNodes.y)*(numNodes.z); i++, pNode++ )
        pNode->MarkAsEmpty();
}

/**************************************************************************\
*
* GetNodeCount
*
\**************************************************************************/

void
NODE_ARRAY::GetNodeCount( IPOINT3D *count )
{
    *count = numNodes;
}

/**************************************************************************\
*
* ChooseRandomDirection
*
* Choose randomnly among the possible directions.  The likelyhood of going
* straight is controlled by weighting it.
*
\**************************************************************************/

int 
NODE_ARRAY::ChooseRandomDirection( IPOINT3D *pos, int dir, int weightStraight )
{
    Node *nNode[NUM_DIRS];
    int numEmpty, newDir;
    int choice;
    Node *straightNode = NULL;
    int emptyDirs[NUM_DIRS];

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::ChooseRandomDirection: invalid dir\n" );

    // Get the neigbouring nodes
    GetNeighbours( pos, nNode );

    // Get node in straight direction if necessary
    if( weightStraight && nNode[dir] && nNode[dir]->IsEmpty() ) {
        straightNode = nNode[dir];
        // if maximum weight, choose and return
        if( weightStraight == MAX_WEIGHT_STRAIGHT ) {
            straightNode->MarkAsTaken();
            return dir;
        }
    } else
        weightStraight = 0;

    // Get directions of possible turns
    numEmpty = GetEmptyTurnNeighbours( nNode, emptyDirs, dir );

    // Make a random choice
    if( (choice = (weightStraight + numEmpty)) == 0 )
        return DIR_NONE;
    choice = ss_iRand( choice );

    if( choice < weightStraight ) {
        straightNode->MarkAsTaken();
        return dir;
    } else {
        // choose one of the turns
        newDir = emptyDirs[choice - weightStraight];
        nNode[newDir]->MarkAsTaken();
        return newDir;
    }
}

/**************************************************************************\
*
* ChoosePreferredDirection
*
* Choose randomnly from one of the supplied preferred directions.  If none
* of these are available, then try and choose any empty direction
*
\**************************************************************************/

int 
NODE_ARRAY::ChoosePreferredDirection( IPOINT3D *pos, int dir, int *prefDirs,
                                      int nPrefDirs )
{
    Node *nNode[NUM_DIRS];
    int numEmpty, newDir;
    int emptyDirs[NUM_DIRS];
    int *pEmptyPrefDirs;
    int i, j;

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::ChoosePreferredDirection : invalid dir\n" );

    // Get the neigbouring nodes
    GetNeighbours( pos, nNode );

    // Create list of directions that are both preferred and empty

    pEmptyPrefDirs = emptyDirs;
    numEmpty = 0;

    for( i = 0, j = 0; (i < NUM_DIRS) && (j < nPrefDirs); i++ ) {
        if( i == *prefDirs ) {
            prefDirs++;
            j++;
            if( nNode[i] && nNode[i]->IsEmpty() ) {
                // add it to list
                *pEmptyPrefDirs++ = i;
                numEmpty++;
            }
        }
    }

    // if no empty preferred dirs, then any empty dirs become preferred
    
    if( !numEmpty ) {
        numEmpty = GetEmptyNeighbours( nNode, emptyDirs );
        if( numEmpty == 0 )
            return DIR_NONE;
    }
                
    // Pick a random dir from the empty set

    newDir = emptyDirs[ss_iRand( numEmpty )];
    nNode[newDir]->MarkAsTaken();
    return newDir;
}

/**************************************************************************\
*
* FindClearestDirection
*
* Finds the direction with the most empty nodes in a line 'searchRadius'
* long.  Does not mark any nodes as taken.
*
\**************************************************************************/

int 
NODE_ARRAY::FindClearestDirection( IPOINT3D *pos )
{
    static Node *neighbNode[NUM_DIRS];
    static int emptyDirs[NUM_DIRS];
    int nEmpty, newDir;
    int maxEmpty = 0;
    int searchRadius = 3;
    int count = 0;
    int i;

    // Get ptrs to neighbour nodes

    GetNeighbours( pos, neighbNode );

    // find empty nodes in each direction

    for( i = 0; i < NUM_DIRS; i ++ ) {
        if( neighbNode[i] && neighbNode[i]->IsEmpty() )
        {
            // find number of contiguous empty nodes along this direction
            nEmpty = GetEmptyNeighboursAlongDir( pos, i, searchRadius );
            if( nEmpty > maxEmpty ) {
                // we have a new winner
                count = 0;
                maxEmpty = nEmpty;
                emptyDirs[count++] = i;
            }
            else if( nEmpty == maxEmpty ) {
                // tied with current max
                emptyDirs[count++] = i;
            }
        }
    }

    if( count == 0 )
        return DIR_NONE;

    // randomnly choose a direction
    newDir = emptyDirs[ss_iRand( count )];

    return newDir;
}
/**************************************************************************\
*
* ChooseNewTurnDirection
*
* Choose a direction to turn
*
* This requires finding a pair of nodes to turn through.  The first node
* is in the direction of the turn from the current node, and the second node
* is at right angles to this at the end position.  The prim will not draw
* through the first node, but may sweep close to it, so we have to mark it
* as taken.
*
* - if next node is free, but there are no turns available, return
*   DIR_STRAIGHT, so the caller can decide what to do in this case
* - The turn possibilities are based on the orientation of the current xc, with
*   4 relative directions to seek turns in.
*
* History
*  Aug. 3, 95 : Marc Fortier [marcfo]
*    - Wrote it
*
\**************************************************************************/

int 
NODE_ARRAY::ChooseNewTurnDirection( IPOINT3D *pos, int dir )
{
    Node *nNode[NUM_DIRS];
    int turns[NUM_DIRS], nTurns;
    IPOINT3D nextPos;
    int numEmpty, newDir;
    Node *nextNode;

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::ChooseNewTurnDirection : invalid dir\n" );

    // First, check if next node along current dir is empty

    if( ! GetNextNodePos( pos, &nextPos, dir ) )
        return DIR_NONE; // node out of bounds or not empty

    // Ok, the next node is free - check the 4 possible turns from here

    nTurns = GetBestPossibleTurns( &nextPos, dir, turns );
    if( nTurns == 0 )
        return DIR_STRAIGHT; // nowhere to turn, but could go straight

    // randomnly choose one of the possible turns
    newDir = turns[ ss_iRand( nTurns ) ];

    SS_ASSERT( (newDir >= 0) && (newDir < NUM_DIRS), 
            "NODE_ARRAY::ChooseNewTurnDirection : invalid newDir\n" );


    // mark taken nodes

    nextNode = GetNode( &nextPos );
    nextNode->MarkAsTaken();

    nextNode = GetNextNode( &nextPos, newDir );

    nextNode->MarkAsTaken();

    return newDir;
}

/**************************************************************************\
*
* GetBestPossibleTurns
*
* From supplied direction and position, figure out which of 4 possible 
* directions are best to turn in.
*
* Turns that have the greatest number of empty nodes after the turn are the
* best, since a pipe is less likely to hit a dead end in this case.
* - We only check as far as 'searchRadius' nodes along each dir.
* - Return direction indices of best possible turns in turnDirs, and return 
*   count of these turns in fuction return value.
*
* History
*  Aug. 7, 95 : Marc Fortier [marcfo]
*    - Wrote it
*
\**************************************************************************/

int 
NODE_ARRAY::GetBestPossibleTurns( IPOINT3D *pos, int dir, int *turnDirs )
{
    Node *neighbNode[NUM_DIRS]; // ptrs to 6 neighbour nodes
    int i, count = 0;
    BOOL check[NUM_DIRS] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
    int nEmpty, maxEmpty = 0;
    int searchRadius = 2;

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::GetBestPossibleTurns : invalid dir\n" );

    GetNeighbours( pos, neighbNode );

    switch( dir ) {
        case PLUS_X:    
        case MINUS_X:
            check[PLUS_X] = FALSE;
            check[MINUS_X] = FALSE;
            break;
        case PLUS_Y:    
        case MINUS_Y:
            check[PLUS_Y] = FALSE;
            check[MINUS_Y] = FALSE;
            break;
        case PLUS_Z:    
        case MINUS_Z:
            check[PLUS_Z] = FALSE;
            check[MINUS_Z] = FALSE;
            break;
    }

    // check approppriate directions
    for( i = 0; i < NUM_DIRS; i ++ ) {
        if( check[i] && neighbNode[i] && neighbNode[i]->IsEmpty() )
        {
            // find number of contiguous empty nodes along this direction
            nEmpty = GetEmptyNeighboursAlongDir( pos, i, searchRadius );
            if( nEmpty > maxEmpty ) {
                // we have a new winner
                count = 0;
                maxEmpty = nEmpty;
                turnDirs[count++] = i;
            }
            else if( nEmpty == maxEmpty ) {
                // tied with current max
                turnDirs[count++] = i;
            }
        }
    }

    return count;
}


/**************************************************************************\
*
* GetNeighbours
*
* Get neigbour nodes relative to supplied position
*
*       - get addresses of the neigbour nodes,
*         and put them in supplied matrix
*       - boundary hits are returned as NULL
*
\**************************************************************************/

void 
NODE_ARRAY::GetNeighbours( IPOINT3D *pos, Node **nNode )
{
    Node *centerNode = GetNode( pos );

    nNode[PLUS_X]  = pos->x == (numNodes.x - 1) ? NULL : 
                                            centerNode + nodeDirInc[PLUS_X];
    nNode[PLUS_Y]  = pos->y == (numNodes.y - 1) ? NULL :
                                            centerNode + nodeDirInc[PLUS_Y];
    nNode[PLUS_Z]  = pos->z == (numNodes.z - 1) ? NULL : 
                                            centerNode + nodeDirInc[PLUS_Z];

    nNode[MINUS_X] = pos->x == 0 ? NULL : centerNode + nodeDirInc[MINUS_X];
    nNode[MINUS_Y] = pos->y == 0 ? NULL : centerNode + nodeDirInc[MINUS_Y];
    nNode[MINUS_Z] = pos->z == 0 ? NULL : centerNode + nodeDirInc[MINUS_Z];
}


/**************************************************************************\
*
* NodeVisited
* 
* Mark the node as non-empty
* 
\**************************************************************************/

void 
NODE_ARRAY::NodeVisited( IPOINT3D *pos )
{
    (GetNode( pos ))->MarkAsTaken();
}

/**************************************************************************\
*
* GetNode
*
* Get ptr to node from position
*
\**************************************************************************/

Node *
NODE_ARRAY::GetNode( IPOINT3D *pos )
{
    return nodes +
           pos->x +
           pos->y * numNodes.x +
           pos->z * numNodes.x * numNodes.y;
}

/**************************************************************************\
*
* GetNextNode
*
* Get ptr to next node from pos and dir
*
\**************************************************************************/

Node *
NODE_ARRAY::GetNextNode( IPOINT3D *pos, int dir )
{
    Node *curNode = GetNode( pos );

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::GetNextNode : invalid dir\n" );

    switch( dir ) {
        case PLUS_X:
            return( pos->x == (numNodes.x - 1) ? NULL : 
                              curNode + nodeDirInc[PLUS_X]);
            break;
        case MINUS_X:
            return( pos->x == 0 ? NULL : 
                              curNode + nodeDirInc[MINUS_X]);
            break;
        case PLUS_Y:
            return( pos->y == (numNodes.y - 1) ? NULL : 
                              curNode + nodeDirInc[PLUS_Y]);
            break;
        case MINUS_Y:
            return( pos->y == 0 ? NULL : 
                              curNode + nodeDirInc[MINUS_Y]);
            break;
        case PLUS_Z:
            return( pos->z == (numNodes.z - 1) ? NULL : 
                              curNode + nodeDirInc[PLUS_Z]);
            break;
        case MINUS_Z:
            return( pos->z == 0 ? NULL : 
                              curNode + nodeDirInc[MINUS_Z]);
            break;
        default:
            return NULL;
    }
}


/**************************************************************************\
*
* GetNextNodePos
*
* Get position of next node from curPos and lastDir
*
* Returns FALSE if boundary hit or node empty
*
\**************************************************************************/

BOOL
NODE_ARRAY::GetNextNodePos( IPOINT3D *curPos, IPOINT3D *nextPos, int dir )
{
    static Node *neighbNode[NUM_DIRS]; // ptrs to 6 neighbour nodes

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::GetNextNodePos : invalid dir\n" );

//mf: don't need to get all neighbours, just one in next direction
    GetNeighbours( curPos, neighbNode );

    *nextPos = *curPos;

    // bail if boundary hit or node not empty
    if( (neighbNode[dir] == NULL) || !neighbNode[dir]->IsEmpty() )
        return FALSE;

    switch( dir ) {
        case PLUS_X:
            nextPos->x = curPos->x + 1;
            break;

        case MINUS_X:
            nextPos->x = curPos->x - 1;
            break;

        case PLUS_Y:
            nextPos->y = curPos->y + 1;
            break;

        case MINUS_Y:
            nextPos->y = curPos->y - 1;
            break;

        case PLUS_Z:
            nextPos->z = curPos->z + 1;
            break;

        case MINUS_Z:
            nextPos->z = curPos->z - 1;
            break;
    }

    return TRUE;
}


/**************************************************************************\
*             
*    GetEmptyNeighbours()
*       - get list of direction indices of empty node neighbours,
*         and put them in supplied matrix
*       - return number of empty node neighbours
*
\**************************************************************************/

int 
NODE_ARRAY::GetEmptyNeighbours( Node **nNode, int *nEmpty )
{
    int i, count = 0;

    for( i = 0; i < NUM_DIRS; i ++ ) {
        if( nNode[i] && nNode[i]->IsEmpty() )
            nEmpty[count++] = i;
    }
    return count;
}

/**************************************************************************\
*             
*    GetEmptyTurnNeighbours()
*       - get list of direction indices of empty node neighbours,
*         and put them in supplied matrix
*       - don't include going straight
*       - return number of empty node neighbours
*
\**************************************************************************/

int 
NODE_ARRAY::GetEmptyTurnNeighbours( Node **nNode, int *nEmpty, int lastDir )
{
    int i, count = 0;

    for( i = 0; i < NUM_DIRS; i ++ ) {
        if( nNode[i] && nNode[i]->IsEmpty() ) {
            if( i == lastDir )
                continue;
            nEmpty[count++] = i;
        }
    }
    return count;
}

/**************************************************************************\
* GetEmptyNeighboursAlongDir
*
* Sort of like above, but just gets one neigbour according to supplied dir
*
* Given a position and direction, find out how many contiguous empty nodes 
* there are in that direction.
* - Can limit search with searchRadius parameter
* - Return contiguous empty node count
*
* History
*  Aug. 12, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

int
NODE_ARRAY::GetEmptyNeighboursAlongDir( IPOINT3D *pos, int dir,
                            int searchRadius )
{
    Node *curNode = GetNode( pos );
    int nodeStride;
    int maxSearch;
    int count = 0;

    SS_ASSERT( (dir >= 0) && (dir < NUM_DIRS), 
            "NODE_ARRAY::GetEmptyNeighboursAlongDir : invalid dir\n" );

    nodeStride = nodeDirInc[dir];

    switch( dir ) {
        case PLUS_X:    
            maxSearch = numNodes.x - pos->x - 1;
            break;
        case MINUS_X:
            maxSearch = pos->x;
            break;
        case PLUS_Y:    
            maxSearch = numNodes.y - pos->y - 1;
            break;
        case MINUS_Y:
            maxSearch = pos->y;
            break;
        case PLUS_Z:    
            maxSearch = numNodes.z - pos->z - 1;
            break;
        case MINUS_Z:
            maxSearch = pos->z;
            break;
    }
    
    if( searchRadius > maxSearch )
        searchRadius = maxSearch;

    if( !searchRadius )
        return 0;

    while( searchRadius-- ) {
        curNode += nodeStride;
        if( ! curNode->IsEmpty() )
            return count;
        count++;
    }
    return count;
}

/**************************************************************************\
* FindRandomEmptyNode
*
* - Search for an empty node to start drawing
* - Return position of empty node in supplied pos ptr
* - Returns FALSE if couldn't find a node
* - Marks node as taken (mf: renam fn to ChooseEmptyNode ?
*
* History
*  July 19, 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

// If random search takes longer than twice the total number
// of nodes, give up the random search.  There may not be any
// empty nodes.

#define INFINITE_LOOP   (2 * NUM_NODE * NUM_NODE * NUM_NODE)

BOOL
NODE_ARRAY::FindRandomEmptyNode( IPOINT3D *pos )
{
    int infLoopDetect = 0;

    while( TRUE ) {

        // Pick a random node.

        pos->x = ss_iRand( numNodes.x );
        pos->y = ss_iRand( numNodes.y );
        pos->z = ss_iRand( numNodes.z );

        // If its empty, we're done.

        if( GetNode(pos)->IsEmpty() ) {
            NodeVisited( pos );
            return TRUE;
        } else {
            // Watch out for infinite loops!  After trying for
            // awhile, give up on the random search and look
            // for the first empty node.

            if ( infLoopDetect++ > INFINITE_LOOP ) {

                // Search for first empty node.

                for ( pos->x = 0; pos->x < numNodes.x; pos->x++ )
                    for ( pos->y = 0; pos->y < numNodes.y; pos->y++ )
                        for ( pos->z = 0; pos->z < numNodes.z; pos->z++ )
                            if( GetNode(pos)->IsEmpty() ) {
                                NodeVisited( pos );
                                return TRUE;
                            }

                // There are no more empty nodes.
                // Reset the pipes and exit.

                return FALSE;
            }
        }
    }
}

/**************************************************************************\
* FindRandomEmptyNode2D
*
* - Like FindRandomEmptyNode, but limits search to a 2d plane of the supplied
*   box.
*
\**************************************************************************/

#define INFINITE_LOOP   (2 * NUM_NODE * NUM_NODE * NUM_NODE)
#define MIN_VAL 1
#define MAX_VAL 0

BOOL
NODE_ARRAY::FindRandomEmptyNode2D( IPOINT3D *pos, int plane, int *box )
{
    int *newx, *newy;
    int *xDim, *yDim;

    switch( plane ) {
        case PLUS_X:
        case MINUS_X:
            pos->x = box[plane];
            newx = &pos->z;
            newy = &pos->y;
            xDim = &box[PLUS_Z]; 
            yDim = &box[PLUS_Y]; 
            break;
        case PLUS_Y:
        case MINUS_Y:
            pos->y = box[plane];
            newx = &pos->x;
            newy = &pos->z;
            xDim = &box[PLUS_X]; 
            yDim = &box[PLUS_Z]; 
            break;
        case PLUS_Z:
        case MINUS_Z:
            newx = &pos->x;
            newy = &pos->y;
            pos->z = box[plane];
            xDim = &box[PLUS_X]; 
            yDim = &box[PLUS_Y]; 
            break;
    }

    int infLoop = 2 * (xDim[MAX_VAL] - xDim[MIN_VAL] + 1) *
                      (yDim[MAX_VAL] - yDim[MIN_VAL] + 1);
    int infLoopDetect = 0;

    while( TRUE ) {

        // Pick a random node.

        *newx = ss_iRand2( xDim[MIN_VAL], xDim[MAX_VAL] );
        *newy = ss_iRand2( yDim[MIN_VAL], yDim[MAX_VAL] );

        // If its empty, we're done.

        if( GetNode(pos)->IsEmpty() ) {
            NodeVisited( pos );
            return TRUE;
        } else {
            // Watch out for infinite loops!  After trying for
            // awhile, give up on the random search and look
            // for the first empty node.

            if ( ++infLoopDetect > infLoop ) {

                // Do linear search for first empty node.

                for ( *newx = xDim[MIN_VAL]; *newx <= xDim[MAX_VAL]; (*newx)++ )
                    for ( *newy = yDim[MIN_VAL]; *newy <= yDim[MAX_VAL]; (*newy)++ )
                        if( GetNode(pos)->IsEmpty() ) {
                            NodeVisited( pos );
                            return TRUE;
                        }

                // There are no empty nodes in this plane.
                return FALSE;
            }
        }
    }
}

/**************************************************************************\
* TakeClosestEmptyNode
*
* - Search for an empty node closest to supplied node position
* - Returns FALSE if couldn't find a node
* - Marks node as taken
* - mf: not completely opimized - if when dilating the box, a side gets
*   clamped against the node array, this side will continue to be searched
*
* History
*  Dec 95 : [marcfo]
*    - Wrote it
*
\**************************************************************************/

static void
DilateBox( int *box, IPOINT3D *bounds );

BOOL
NODE_ARRAY::TakeClosestEmptyNode( IPOINT3D *newPos, IPOINT3D *pos )
{
    static int searchRadius = SS_MAX( numNodes.x, numNodes.y ) / 3;

    // easy out
    if( GetNode(pos)->IsEmpty() ) {
        NodeVisited( pos );
        *newPos = *pos;
        return TRUE;
    }

    int box[NUM_DIRS] = {pos->x, pos->x, pos->y, pos->y, pos->z, pos->z};
    int clip[NUM_DIRS] = {0};

    // do a random search on successively larger search boxes
    for( int i = 0; i < searchRadius; i++ ) {
        // Increase box size
        DilateBox( box, &numNodes );
        // start looking in random 2D face of the box
        int dir = ss_iRand( NUM_DIRS );
        for( int j = 0; j < NUM_DIRS; j++, dir = (++dir == NUM_DIRS) ? 0 : dir ) {
            if( FindRandomEmptyNode2D( newPos, dir, box ) )
                return TRUE;
        }
    }

    // nothing nearby - grab a random one
    return FindRandomEmptyNode( newPos );
}

/**************************************************************************\
* DilateBox
*
* - Increase box radius without exceeding bounds
*
\**************************************************************************/

static void
DilateBox( int *box, IPOINT3D *bounds )
{
    int *min = (int *) &box[MINUS_X];
    int *max = (int *) &box[PLUS_X];
    int *boundMax = (int *) bounds;
    // boundMin always 0

    for( int i = 0; i < 3; i ++, min+=2, max+=2, boundMax++ ) {
        if( *min > 0 )
            (*min)--;
        if( *max < (*boundMax - 1) )
            (*max)++;
    }
}
