/*
 *	This module contains the definitions for the inline functions used by the
 *	name undecorator.  It is intended that this file should be included
 *	somewhere in the source file for the undecorator to maximise the chance
 *	that they will be truly inlined.
 */

//	The following class is a special node class, used in the implementation
//	of the internal chaining mechanism of the 'DName's

class	charNode;
class	pcharNode;
class	pDNameNode;
class	DNameStatusNode;


#if	( NO_VIRTUAL )
enum	NodeType
{
	charNode_t,
	pcharNode_t,
	pDNameNode_t,
	DNameStatusNode_t

};
#endif	// NO_VIRTUAL


class	DNameNode
{
private:

#if	NO_VIRTUAL
		NodeType			typeIndex;
#endif	// NO_VIRTUAL

		DNameNode *			next;

protected:

#if	( !NO_VIRTUAL )
					__near	DNameNode ();
#else	// } elif NO_VIRTUAL {
					__near	DNameNode ( NodeType );
#endif	// NO_VIRTUAL

					__near	DNameNode ( const DNameNode & );

public:

virtual	int			__near	length () const PURE;
virtual	pchar_t		__near	getString ( pchar_t, int ) const PURE;
		DNameNode *	__near	clone ();
		DNameNode *	__near	nextNode () const;

		DNameNode &	__near	operator += ( DNameNode * );

};


class	charNode		: public DNameNode
{
private:
		char				me;

public:
					__near	charNode ( char );

virtual	int			__near	length () const;
virtual	pchar_t		__near	getString ( pchar_t, int ) const;

};


class	pcharNode		: public DNameNode
{
private:
		pchar_t				me;
		int					myLen;

public:
					__near	pcharNode ( pcchar_t, int = 0 );

virtual	int			__near	length () const;
virtual	pchar_t		__near	getString ( pchar_t, int ) const;

};


class	pDNameNode		: public DNameNode
{
private:
		DName *				me;

public:
					__near	pDNameNode ( DName * );

virtual	int			__near	length () const;
virtual	pchar_t		__near	getString ( pchar_t, int ) const;

};


class	DNameStatusNode	: public DNameNode
{
private:
#define	TruncationMessage		(" ?? ")
#define	TruncationMessageLength	(4)

		DNameStatus			me;
		int					myLen;

public:
					__near	DNameStatusNode ( DNameStatus );

virtual	int			__near	length () const;
virtual	pchar_t		__near	getString ( pchar_t, int ) const;

};



//	Memory allocation functions
			
inline	void __far *	__near __pascal	operator new ( unsigned int sz, HeapManager &, int noBuffer )
{	return	heap.getMemory ( sz, noBuffer );	}

void __far *	__near	HeapManager::getMemory ( unsigned int sz, int noBuffer )
{
	//	Align the allocation on an appropriate boundary

	sz	= (( sz + PACK_SIZE-1 ) & ~(PACK_SIZE-1) );

	if	( noBuffer )
		return	( *pOpNew )( sz );
	else
	{
		//	Handler a potential request for no space

		if	( !sz )
			sz	= 1;

		if	( blockLeft < sz )
		{
			//	Is the request greater than the largest buffer size ?

			if	( sz > memBlockSize )
				return	0;		// If it is, there is nothing we can do


			//	Allocate a new block

			Block *	pNewBlock	= rnew Block;


			//	Did the allocation succeed ?  If so connect it up

			if	( pNewBlock )
			{
				//	Handle the initial state

				if	( tail )
					tail	= tail->next	= pNewBlock;
				else
					head	= tail			= pNewBlock;

				//	Compute the remaining space

				blockLeft	= memBlockSize - sz;

			}	// End of IF then
			else
				return	0;		// Oh-oh!  Memory allocation failure

		}	// End of IF then
		else
			blockLeft	-= sz;	// Deduct the allocated amount

		//	And return the buffer address

		return	&( tail->memBlock[ blockLeft ]);

	}	// End of IF else
}	// End of "HeapManager" FUNCTION "getMemory(unsigned int,int)"




//	Friend functions of 'DName'

inline DName	__near __pascal	operator + ( char c, const DName & rd )
{	return	DName ( c ) + rd;	}

inline DName	__near __pascal	operator + ( DNameStatus st, const DName & rd )
{	return	DName ( st ) + rd;	}

inline DName	__near __pascal	operator + ( pcchar_t s, const DName & rd )
{	return	DName ( s ) + rd;	}


//	The 'DName' constructors

inline		__near	DName::DName ()					{	node	= 0;	stat	= DN_valid;	isIndir	= 0;	isAUDC	= 0; isAUDTThunk = 0;	}
inline		__near	DName::DName ( DNameNode * pd )	{	node	= pd;	stat	= DN_valid;	isIndir	= 0;	isAUDC	= 0; isAUDTThunk = 0;	}

__near	DName::DName ( char c )
{
	stat	= DN_valid;
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;
	node	= 0;

	//	The NULL character is boring, do not copy

	if	( c )
		doPchar ( &c, 1 );

}	// End of "DName" CONSTRUCTOR '(char)'


#if	1
inline __near	DName::DName ( const DName & rd )
{
	stat	= rd.stat;
	isIndir	= rd.isIndir;
	isAUDC	= rd.isAUDC;
	isAUDTThunk = rd.isAUDTThunk;
	node	= rd.node;

}	// End of "DName" CONSTRUCTOR '(const DName&)'
#endif


__near	DName::DName ( DName * pd )
{
	if	( pd )
	{
		node	= gnew pDNameNode ( pd );
		stat	= ( node ? DN_valid : DN_error );

	}	// End of IF else
	else
	{
		stat	= DN_valid;
		node	= 0;

	}	// End of IF else

	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;

}	// End of "DName" CONSTRUCTOR '( DName* )'


__near	DName::DName ( pcchar_t s )
{
	stat	= DN_valid;
	node	= 0;
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;

	if	( s )
		doPchar ( s, strlen ( s ));

}	// End of "DName" CONSTRUCTOR '(pcchar_t)'


__near	DName::DName ( pcchar_t & name, char terminator )
{
	stat	= DN_valid;
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;
	node	= 0;

	//	Is there a string ?

	if	( name )
		if	( *name )
		{
			int	len	= 0;


			//	How long is the string ?

			for	( pcchar_t s = name; *name && ( *name != terminator ); name++ )
				if	( isValidIdentChar ( *name ))
					len++;
				else
				{
					stat	= DN_invalid;

					return;

				}	// End of IF else

			//	Copy the name string fragment

			doPchar ( s, len );

			//	Now gobble the terminator if present, handle error conditions

			if	( *name )
			{
				if	( *name++ != terminator )
				{
					stat	= DN_error;
					node	= 0;

				}	// End of IF then
				else
					stat	= DN_valid;

			}	// End of IF then
			elif	( status () == DN_valid )
				stat	= DN_truncated;

		}	// End of IF then
		else
			stat	= DN_truncated;
	else
		stat	= DN_invalid;

}	// End of "DName" CONSTRUCTOR '(pcchar_t&,char)'


__near	DName::DName ( unsigned long num )
{
	char	buf[ 11 ];
	char *	pBuf	= buf + 10;


	stat	= DN_valid;
	node	= 0;
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;

	//	Essentially, 'ultoa ( num, buf, 10 )' :-

	*pBuf	= 0;

	do
	{
		*( --pBuf )	= (char)(( num % 10 ) + '0' );
		num			/= 10UL;

	}	while	( num );

	doPchar ( pBuf, ( 10 - ( pBuf - buf )));

}	// End of "DName" CONSTRUCTOR '(unsigned long)'


__near	DName::DName ( DNameStatus st )
{
	stat	= ((( st == DN_invalid ) || ( st == DN_error )) ? st : DN_valid );
	node	= gnew DNameStatusNode ( st );
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;

	if	( !node )
		stat	= DN_error;

}	// End of "DName" CONSTRUCTOR '(DNameStatus)'



//	Now the member functions for 'DName'

int		__near	DName::isValid () const		{	return	(( status () == DN_valid ) || ( status () == DN_truncated ));	}
int		__near	DName::isEmpty () const		{	return	(( node == 0 ) || !isValid ());	}

inline	DNameStatus	__near	DName::status () const	{	return	(DNameStatus)stat;	}	// The cast is to keep Glockenspiel quiet

inline	DName &	__near	DName::setPtrRef ()			{	isIndir	= 1;	return	*this;	}
inline	int		__near	DName::isPtrRef () const	{	return	isIndir;	}
inline	int		__near	DName::isUDC () const		{	return	( !isEmpty () && isAUDC );	}
inline	void	__near	DName::setIsUDC ()			{	if	( !isEmpty ())	isAUDC	= TRUE;	}
inline	int		__near	DName::isUDTThunk () const		{	return	( !isEmpty () && isAUDTThunk );	}
inline	void	__near	DName::setIsUDTThunk ()			{	if	( !isEmpty ())	isAUDTThunk	= TRUE;	}

int	__near	DName::length () const
{
	int	len	= 0;


	if	( !isEmpty ())
		for	( DNameNode * pNode = node; pNode; pNode = pNode->nextNode ())
			len	+= pNode->length ();

	return	len;

}	// End of "DName" FUNCTION "length"


pchar_t	__near	DName::getString ( pchar_t buf, int max ) const
{
	if		( !isEmpty ())
	{
		//	Does the caller want a buffer allocated ?

		if	( !buf )
		{
			max	= length () + 1;
			buf	= gnew char[ max ];	// Get a buffer big enough

		}	// End of IF then

		//	If memory allocation failure, then return no buffer

		if	( buf )
		{
			//	Now, go through the process of filling the buffer (until max is reached)

			int			curLen	= max;
			DNameNode *	curNode	= node;
			pchar_t		curBuf	= buf;


			while	( curNode && ( curLen > 0 ))
			{
				int		fragLen	= curNode->length ();
				pchar_t	fragBuf	= 0;


				//	Skip empty nodes

				if	( fragLen )
				{
					//	Handle buffer overflow

					if	(( curLen - fragLen ) < 0 )
						fragLen	= curLen;

					//	Now copy 'len' number of bytes of the piece to the buffer

					fragBuf	= curNode->getString ( curBuf, fragLen );

					//	Should never happen, but handle it anyway

					if	( fragBuf )
					{
						//	Update string position

						curLen	-= fragLen;
						curBuf	+= fragLen;

					}	// End of IF
				}	// End of IF

				//	Move on to the next name fragment

				curNode	= curNode->nextNode ();

			}	// End of WHILE

			*curBuf	= 0;	// Always NULL terminate the resulting string

		}	// End of IF
	}	// End of IF then
	elif	( buf )
		*buf	= 0;

	//	Return the buffer

	return	buf;

}	// End of "DName" FUNCTION "getString(pchar_t,int)"


DName	__near	DName::operator + ( char ch ) const
{
	DName	local ( *this );


	if	( local.isEmpty ())
		local	= ch;
	else
		local	+= ch;

	//	And return the newly formed 'DName'

	return	local;

}	// End of "DName" OPERATOR "+(char)"


DName	__near	DName::operator + ( pcchar_t str ) const
{
	DName	local ( *this );


	if	( local.isEmpty ())
		local	= str;
	else
		local	+= str;

	//	And return the newly formed 'DName'

	return	local;

}	// End of "DName" OPERATOR "+(pcchar_t)"


DName	__near	DName::operator + ( const DName & rd ) const
{
	DName	local ( *this );


	if		( local.isEmpty ())
		local	= rd;
	elif	( rd.isEmpty ())
		local	+= rd.status ();
	else
		local	+= rd;

	//	And return the newly formed 'DName'

	return	local;

}	// End of "DName" OPERATOR "+(const DName&)"


DName	__near	DName::operator + ( DName * pd ) const
{
	DName	local ( *this );


	if	( local.isEmpty ())
		local	= pd;
	else
		local	+= pd;

	//	And return the newly formed 'DName'

	return	local;

}	// End of "DName" OPERATOR "+(DName*)"


DName	__near	DName::operator + ( DNameStatus st ) const
{
	DName	local ( *this );


	if	( local.isEmpty ())
		local	= st;
	else
		local	+= st;

	//	And return the newly formed 'DName'

	return	local;

}	// End of "DName" OPERATOR "+(DNameStatus)"



DName &	__near	DName::operator += ( char ch )
{
	if	( ch )
		if	( isEmpty ())
			*this	= ch;
		else
		{
			node	= node->clone ();

			if	( node )
				*node	+= gnew charNode ( ch );
			else
				stat	= DN_error;

		}	// End of IF

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR "+=(char)"


DName &	__near	DName::operator += ( pcchar_t str )
{
	if	( str && *str )
		if	( isEmpty ())
			*this	= str;
		else
		{
			node	= node->clone ();

			if	( node )
				*node	+= gnew pcharNode ( str );
			else
				stat	= DN_error;

		}	// End of IF

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR "+=(pcchar_t)"


DName &	__near	DName::operator += ( const DName & rd )
{
	if	( rd.isEmpty ())
		*this	+= rd.status ();
	else
		if	( isEmpty ())
			*this	= rd;
		else
		{
			node	= node->clone ();

			if	( node )
				*node	+= rd.node;
			else
				stat	= DN_error;

		}	// End of IF

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR "+=(const DName&)"


DName &	__near	DName::operator += ( DName * pd )
{
	if	( pd )
		if		( isEmpty ())
			*this	= pd;
		elif	(( pd->status () == DN_valid ) || ( pd->status () == DN_truncated ))
		{
			DNameNode *	pNew	= gnew pDNameNode ( pd );


			if	( pNew )
			{
				node	= node->clone ();

				if	( node )
					*node	+= pNew;

			}	// End of IF then
			else
				node	= 0;

			if	( !node )
				stat	= DN_error;

		}	// End of IF then
		else
			*this	+= pd->status ();

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR "+=(DName*)"


DName &	__near	DName::operator += ( DNameStatus st )
{
	if	( isEmpty () || (( st == DN_invalid ) || ( st == DN_error )))
		*this	= st;
	else
	{
		DNameNode *	pNew	= gnew DNameStatusNode ( st );


		if	( pNew )
		{
			node	= node->clone ();

			if	( node )
				*node	+= pNew;

		}	// End of IF then
		else
			node	= 0;

		if	( !node )
			stat	= DN_error;

	}	// End of IF else

	//	Return self

	return	*this;

}	// End of "DName" OPERATOR "+=(DNameStatus)"



DName &	__near	DName::operator |= ( const DName & rd )
{
	//	Attenuate the error status.  Always becomes worse.  Don't propogate truncation

	if	(( status () != DN_error ) && !rd.isValid ())
		stat	= rd.status ();

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR '|=(const DName&)'



DName &	__near	DName::operator = ( char ch )
{
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;

	doPchar ( &ch, 1 );

	return	*this;

}	// End of "DName" OPERATOR '=(char)'


DName &	__near	DName::operator = ( pcchar_t str )
{
	isIndir	= 0;
	isAUDC	= 0;
	isAUDTThunk = 0;

	doPchar ( str, strlen ( str ));

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR '=(pcchar_t)'


DName &	__near	DName::operator = ( const DName & rd )
{
	if	(( status () == DN_valid ) || ( status () == DN_truncated ))
	{
		stat	= rd.stat;
		isIndir	= rd.isIndir;
		isAUDC	= rd.isAUDC;
		isAUDTThunk = rd.isAUDTThunk;
		node	= rd.node;

	}	// End of IF

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR '=(const DName&)'


DName &	__near	DName::operator = ( DName * pd )
{
	if	(( status () == DN_valid ) || ( status () == DN_truncated ))
		if	( pd )
		{
			isIndir	= 0;
			isAUDC	= 0;
			isAUDTThunk = 0;
			node	= gnew pDNameNode ( pd );

			if	( !node )
				stat	= DN_error;

		}	// End of IF then
		else
			*this	= DN_error;

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR '=(DName*)'


DName &	__near	DName::operator = ( DNameStatus st )
{
	if	(( st == DN_invalid ) || ( st == DN_error ))
	{
		node	= 0;

		if	( status () != DN_error )
			stat	= st;

	}	// End of IF then
	elif	(( status () == DN_valid ) || ( status () == DN_truncated ))
	{
		isIndir	= 0;
		isAUDC	= 0;
		isAUDTThunk = 0;
		node	= gnew DNameStatusNode ( st );

		if	( !node )
			stat	= DN_error;

	}	// End of ELIF then

	//	And return self

	return	*this;

}	// End of "DName" OPERATOR '=(DNameStatus)'


//	Private implementation functions for 'DName'

void	__near	DName::doPchar ( pcchar_t str, int len )
{
	if	( !(( status () == DN_invalid ) || ( status () == DN_error )))
		if		( node )
			*this	= DN_error;
		elif	( str && len )
		{
			//	Allocate as economically as possible

			switch	( len )
			{
			case 0:
					stat	= DN_error;
				break;

			case 1:
					node	= gnew charNode ( *str );

					if	( !node )
						stat	= DN_error;
				break;

			default:
					node	= gnew pcharNode ( str, len );

					if	( !node )
						stat	= DN_error;
				break;

			}	// End of SWITCH
		}	// End of ELIF
		else
			stat	= DN_invalid;

}	// End of "DName" FUNCTION "doPchar(pcchar_t,int)"



//	The member functions for the 'Replicator'

inline	int	__near	Replicator::isFull () const		{	return	( index == 9 );	}
inline	__near		Replicator::Replicator ()
:	ErrorDName ( DN_error ), InvalidDName ( DN_invalid )
{	index	= -1;	}



Replicator &	__near	Replicator::operator += ( const DName & rd )
{
	if	( !isFull () && !rd.isEmpty ())
	{
		DName *	pNew	= gnew DName ( rd );


		//	Don't update if failed

		if	( pNew )
			dNameBuffer[ ++index ]	= pNew;

	}	// End of IF

	return	*this;

}	// End of "Replicator" OPERATOR '+=(const DName&)'


const DName &	__near	Replicator::operator [] ( int x ) const
{
	if		(( x < 0 ) || ( x > 9 ))
		return	ErrorDName;
	elif	(( index == -1 ) || ( x > index ))
		return	InvalidDName;
	else
		return	*dNameBuffer[ x ];

}	// End of "Replicator" OPERATOR '[](int)'



//	The member functions for the 'DNameNode' classes

#if	( !NO_VIRTUAL )
__near	DNameNode::DNameNode ()
#else	// } elif NO_VIRTUAL {
__near	DNameNode::DNameNode ( NodeType ndTy )
:	typeIndex ( ndTy )
#endif	// NO_VIRTUAL
{	next	= 0;	}

inline	__near	DNameNode::DNameNode ( const DNameNode & rd )	{	next	= (( rd.next ) ? rd.next->clone () : 0 );	}

inline	DNameNode *	__near	DNameNode::nextNode () const		{	return	next;	}

DNameNode *	__near	DNameNode::clone ()
{
	return	gnew pDNameNode ( gnew DName ( this ));
}

#if	( NO_VIRTUAL )
int	__near	DNameNode::length () const
{	//	Pure function, should not be called

	switch	( typeIndex )
	{
	case charNode_t:
		return	((charNode*)this )->length ();

	case pcharNode_t:
		return	((pcharNode*)this )->length ();

	case pDNameNode_t:
		return	((pDNameNode*)this )->length ();

	case DNameStatusNode_t:
		return	((DNameStatusNode*)this )->length ();

	}	// End of SWITCH

	return	0;
}


pchar_t	__near	DNameNode::getString ( pchar_t s, int l ) const
{	//	Pure function, should not be called

	switch	( typeIndex )
	{
	case charNode_t:
		return	((charNode*)this )->getString ( s, l );

	case pcharNode_t:
		return	((pcharNode*)this )->getString ( s, l );

	case pDNameNode_t:
		return	((pDNameNode*)this )->getString ( s, l );

	case DNameStatusNode_t:
		return	((DNameStatusNode*)this )->getString ( s, l );

	}	// End of SWITCH

	return	0;
}
#endif	// NO_VIRTUAL


DNameNode &	__near	DNameNode::operator += ( DNameNode * pNode )
{
	if	( pNode )
	{
		if	( next )
		{
			//	Skip to the end of the chain

			for	( DNameNode* pScan = next; pScan->next; pScan = pScan->next )
				;

			//	And append the new node

			pScan->next	= pNode;

		}	// End of IF then
		else
			next	= pNode;

	}	// End of IF

	//	And return self

	return	*this;

}	// End of "DNameNode" OPERATOR '+=(DNameNode*)'



//	The 'charNode' virtual functions

__near	charNode::charNode ( char ch )
#if	( NO_VIRTUAL )
:	DNameNode ( charNode_t )
#endif	// NO_VIRTUAL
{	me	= ch;	}

inline	int	__near	charNode::length () const		{	return	1;	}

pchar_t	__near	charNode::getString ( pchar_t buf, int len ) const
{
	if	( buf && len )
		*buf	= me;
	else
		buf		= 0;

	//	Now return the character

	return	buf;

}	// End of "charNode" FUNCTION "getString(pchar_t,int)"



//	The 'pcharNode' virtual functions

inline	int	__near	pcharNode::length () const		{	return	myLen;	}


__near	pcharNode::pcharNode ( pcchar_t str, int len )
#if ( NO_VIRTUAL )
:	DNameNode ( pcharNode_t )
#endif	// NO_VIRTUAL
{
	//	Get length if not supplied

	if	( !len && str )
		len	= strlen ( str );

	//	Allocate a new string buffer if valid state

	if	( len && str )
	{
		me		= gnew char[ len ];
		myLen	= len;

		if	( me )
			strncpy ( me, str, len );

	}	// End of IF then
	else
	{
		me		= 0;
		myLen	= 0;

	}	// End of IF else
}	// End of "pcharNode" CONSTRUCTOR '(pcchar_t,int)'


pchar_t	__near	pcharNode::getString ( pchar_t buf, int len ) const
{
	//	Use the shorter of the two lengths (may not be NULL terminated)

	if	( len > pcharNode::length ())
		len	= pcharNode::length ();

	//	Do the copy as appropriate

	return	(( me && buf && len ) ? strncpy ( buf, me, len ) : 0 );

}	// End of "pcharNode" FUNCTION "getString(pchar_t,int)"



//	The 'pDNameNode' virtual functions

__near	pDNameNode::pDNameNode ( DName * pName )
#if	( NO_VIRTUAL )
:	DNameNode ( pDNameNode_t )
#endif	// NO_VIRTUAL
{	me	= (( pName && (( pName->status () == DN_invalid ) || ( pName->status () == DN_error ))) ? 0 : pName );	}

inline	int	__near	pDNameNode::length () const					{	return	( me ? me->length () : 0 );	}

pchar_t	__near	pDNameNode::getString ( pchar_t buf, int len ) const
{	return	(( me && buf && len ) ? me->getString ( buf, len ) : 0 );	}



//	The 'DNameStatusNode' virtual functions

__near	DNameStatusNode::DNameStatusNode ( DNameStatus stat )
#if	( NO_VIRTUAL )
:	DNameNode ( DNameStatusNode_t )
#endif	// NO_VIRTUAL
{	me	= stat;	myLen	= (( me == DN_truncated ) ? TruncationMessageLength : 0 );	}

inline	int	__near	DNameStatusNode::length () const	{	return	myLen;	}

pchar_t	__near	DNameStatusNode::getString ( pchar_t buf, int len ) const
{
	//	Use the shorter of the two lengths (may not be NULL terminated)

	if	( len > DNameStatusNode::length ())
		len	= DNameStatusNode::length ();

	//	Do the copy as appropriate

	return	((( me == DN_truncated ) && buf && len ) ? strncpy ( buf, TruncationMessage, len ) : 0 );

}	// End of "DNameStatusNode" FUNCTION "getString(pchar_t,int)"



static	unsigned int	__near __pascal	strlen ( pcchar_t str )
{
	for	( unsigned int len = 0; *str; str++ )
		len++;

	return	len;

}	// End of FUNCTION "strlen"


static	pchar_t			__near __pascal	strncpy ( pchar_t dst, pcchar_t src, unsigned int len )
{
	for	( char __far * d = dst; ( len && ( *d = *src )); d++, src++, len-- )
		;

	return	dst;

}	// End of FUNCTION "strncpy"
