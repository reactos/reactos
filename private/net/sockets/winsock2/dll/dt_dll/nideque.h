/*++
  
  Copyright (c) 1995 Intel Corp
  
  File Name:
  
    nideque.h
  
  Abstract:
  
    Implements Deque structures.  These are double linked lists  that
    can place and remove data from the beginning and end. This file
    also contains an iterator for Deques.  Important to notice is that
    these are non-intrusive deques.  Meaning the use of these classes
    does not need to insert pointers into there classes to use these.
  
  Author:
    
    Mark Hamilton
  
--*/

#ifndef _NIDEQUE_H_
#define _NIDEQUE_H_

#include "nowarn.h"  /* turn off benign warnings */
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_   /* Prevent inclusion of winsock.h in windows.h */
#endif
#include <windows.h>
#include "nowarn.h"  /* some warnings may have been turned back on */
#include "huerror.h"

template<class T> class NIDeque_c;
template<class T> class NIDequeIter_c;

// Class Name:     NILNode_c
// Purpose:  Simply holds data and pointer to form a double linked
// list. 
// Context:  Can be used anywhere.
template<class T> class NILNode_c {
    friend class NIDeque_c<T>;
    friend class NIDequeIter_c<T>;

    private:
        T		    Data;
        NILNode_c	*Next,
			        *Back;

    public:
        NILNode_c();
};

template<class T> NILNode_c<T>::NILNode_c()
{
    // Make sure the members are correctly initialized.
    Next = NULL;
    Back = NULL;
} // NILNode_c::NILNode_c



// Class Name:     NIDeque_c   
// Purpose:  Non-intrusize deque                                       
// Context:  Can be used anywhere.
template<class T> class NIDeque_c {
    friend class NIDequeIter_c<T>;

    private:
      NILNode_c<T>	*Root,
                	*Tail;   
    public: 
                NIDeque_c();
        	~NIDeque_c();
        BOOL	RemoveFromFront(T &data);    
        BOOL	RemoveFromBack(T &data);
        BOOL	InsertIntoFront(T data);
        BOOL	InsertIntoBack(T data);
        BOOL    GetFromFront(T &data);
	BOOL    GetFromBack(T &data);
	BOOL    IsEmpty();
};




/*++
  
  NIDeque_c::NIDeque_c()
  
  Function Description:
  
      Constructor
  
  Arguments:
  
      None.
  
  Return Value:
  
      None.
  
--*/                                            
template<class T> NIDeque_c<T>::NIDeque_c()
{
    Root = NULL;
    Tail = NULL;
} // End of NINIDeque_c::NIDeque_c 





/*++
  
  NIDeque_c::~NIDeque_c()
  
  Function Description:
  
      Destructor
  
  Arguments:
  
      None.
  
  Return Value:
  
      None.
  
--*/
template<class T> NIDeque_c<T>::~NIDeque_c()
{                                            
    NILNode_c<T>    *bptr    = NULL,
            	    *cptr    = NULL;

    for(bptr=NULL,cptr=Root;cptr;bptr=cptr,cptr=cptr->Next){  
        delete bptr;
    }                                           
    delete bptr;
}                




/*++
  
  NIDeque_c::InsertIntoFront()
  
  Function Description:
  
      Inserts a piece of data onto the linked list.
  
  Arguments:
  
      Data -- Data to put on the linked list.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::InsertIntoFront(T Data)
{       
    NILNode_c<T>	*nptr    = NULL;

    /*  If the list contains something then move the root pointer down.
    //  Otherwise, put this new object on the root.
    */
    if(!(nptr = new NILNode_c<T>)){
         HUSetLastError(ALLOCERROR);
         return FALSE;
    }                 

    nptr->Data = Data;

    if(Root){
        nptr->Next = Root;
        Root->Back = nptr;
        Root = nptr;
    }else{    
        Root = nptr;
        Tail = nptr;
    }
    return TRUE;
} // End of NIDeque_c::InsFront




/*++
  
  NIDeque_c::RemoveFromFront()
  
  Function Description:
  
      Removes a piece of data from the front of the linked list.
  
  Arguments:
  
      Data -- Data to be taken off of the linked list.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::RemoveFromFront(T &Data)
{           
    NILNode_c<T>    *nptr    = NULL;   
    
    if(Root){                 
        nptr = Root;
        Root = Root->Next;
        if(Root){
            Root->Back = NULL; 
        }else{
            Tail = NULL;
        }
        nptr->Next = NULL;
        nptr->Back = NULL; 
        Data = nptr->Data;
        delete nptr;
        return TRUE;
    }else{
        Data = NULL;
        return FALSE;
    }
} // End of NIDeque_c::RemFront



                                          
/*++
  
  NIDeque_c::InsertIntoBack()
  
  Function Description:
  
      Inserts a piece of data onto the back of the linked list.
  
  Arguments:
  
      Data -- Data to be taken off of the linked list.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::InsertIntoBack(T Data)
{       
    NILNode_c<T>    *nptr    = NULL;
    if(!(nptr = new NILNode_c<T>)){
        HUSetLastError(ALLOCERROR);
        return FALSE;
    }    

    nptr->Data = Data;

    if(Root){
        nptr->Back = Tail;
        Tail->Next = nptr;
        Tail = nptr;
    }else{    
        Root = nptr;
        Tail = nptr;
    }
    return TRUE;
} // End of NIDeque_c::InsBack




/*++
  
  NIDeque_c::RemoveFromBack()
  
  Function Description:
  
      Removes a piece of data from the back of the linked list.
  
  Arguments:
  
      Data -- Data to be taken off of the linked list.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::RemoveFromBack(T &Data)
{
    NILNode_c<T>    *nptr    = NULL;

    if(Tail){
        nptr = Tail;
        Tail = Tail->Back;
        if(Tail){
            Tail->Next = NULL;
        }else{
	    Root = NULL;
	}
        nptr->Next = NULL;
        nptr->Back = NULL;
        Data = nptr->Data;
        delete nptr;
        return TRUE;
    }else{
        Data = NULL;
        return FALSE;
    }
}




/*++
  
  NIDeque_c::GetFromFront()
  
  Function Description:
  
      Gets a piece of data from the front of the linked list.
  
  Arguments:
  
      Data -- Data to be taken off of the linked list.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::GetFromFront(T &Data)
{           
    NILNode_c<T>    *nptr    = NULL;   
    
    if(Root){                 
        Data = Root->Data;
        return TRUE;
    }else{
        Data = NULL;
        return FALSE;
    }
} // End of NIDeque_c::GetFromFront




/*++
  
  NIDeque_c::GetFromBack()
  
  Function Description:
  
      Gets a piece of data from the back of the linked list.
  
  Arguments:
  
      Data -- Data to be taken off of the linked list.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::GetFromBack(T &Data)
{           
    NILNode_c<T>    *nptr    = NULL;   
    
    if(Tail){                 
        Data = Tail->Data;
        return TRUE;
    }else{
        Data = NULL;
        return FALSE;
    }
} // End of NIDeque_c::RemFront




/*++
  
  NIDeque_c::IsEmpty()
  
  Function Description:
  
      Determines whether a deques is empty
  
  Arguments:
  
      None.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDeque_c<T>::IsEmpty()
{           
    NILNode_c<T>    *nptr    = NULL;   
    
    if(Root){               
        return FALSE;
    }else{
        return TRUE;
    }
} // End of NIDeque_c::IsEmpty


// Name:     NIDequeIter_c
// Purpose:  An iterator for the Deque_c class.                        
// Context:  Of course only with Deque_c
template<class T> class NIDequeIter_c {
    private:
	    NIDeque_c<T>  *NIDeque;
        NILNode_c<T>  *Current;

    private:
        void	RemoveData(NILNode_c<T> *cptr,T &data);

    protected: // Derived class interface
	inline NILNode_c<T> *GetCurrent();
	inline NIDeque_c<T> *GetNIDeque();

    public: 
				NIDequeIter_c();
                NIDequeIter_c(NIDeque_c<T> &ANIDeque);
        	    ~NIDequeIter_c();
	    BOOL	Initialize(NIDeque_c<T> &ANIDeque);
        BOOL	First(T &data);
        BOOL	Next(T &data);
	    BOOL	Last(T &data);
        BOOL	Back(T &data);
	    BOOL	Remove(T &data);
	    BOOL	Replace(T &src,T chg);
};

//
// Private members                                             
//


/*++
  
  NIDequeIter_c::RemoveData()
  
  Function Description:
  
      To remove all of the data from the linked list.
  
  Arguments:
  
      cptr -- Current pointer in the linked list.
      
      ret_data -- Return the data before deleting.
  
  Return Value:
  
      None.
  
--*/
template<class T> void NIDequeIter_c<T>::RemoveData(NILNode_c<T> *cptr,
                                                    T &ret_data)
{
    NILNode_c<T>  *bptr    = NULL,
            	  *nptr    = NULL,
            	  *fptr    = NULL;

    if(Current == cptr){
        Current = cptr->Next;
    }
    fptr = cptr;
    bptr = cptr->Back;
    nptr = cptr->Next;
    if(bptr != NULL){
        bptr->Next = nptr;
    }                      
    if(nptr != NULL){
        nptr->Back = bptr;
    }
    if(fptr == NIDeque->Root){
        NIDeque->Root = nptr;
    }          
    if(fptr == NIDeque->Tail){
        NIDeque->Tail = bptr;
    }                                             
    if(ret_data){
        ret_data = fptr->Data;
    }
    delete fptr;
} // End of NINIDeque_c::RemoveData


//
// Public members                                             
//


/*++
  
  NIDequeIter_c::NIDequeIter_c()
  
  Function Description:
  
      Constructor.
  
  Arguments:
  
      ANIDeque -- The deque to iterate over.
  
  Return Value:
  
      None.
  
--*/
template<class T> NIDequeIter_c<T>::NIDequeIter_c(NIDeque_c<T> &ANIDeque) 
{
    NIDeque = &ANIDeque;
    Current = NULL;
}



template<class T> NIDequeIter_c<T>::NIDequeIter_c() 
{
    NIDeque = NULL;
    Current = NULL;
}




/*++
  
  NIDequeIter_c::~NIDequeIter_c()
  
  Function Description:
  
      Destructor.
  
  Arguments:
  
      None.
  
  Return Value:
  
      None.
  
--*/
template<class T> NIDequeIter_c<T>::~NIDequeIter_c()
{
}



/*++
  
  NIDequeIter_c::Initialize()
  
  Function Description:
  
      Initializes the iterator.
  
  Arguments:
  
      ANIDeque -> The deque to iterate over.
  
  Return Value:
  
      None.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::Initialize(NIDeque_c<T> &ANIDeque) 
{
    NIDeque = &ANIDeque;
    Current = NULL;
    return TRUE;
}




/*++
  
  NIDequeIter_c::First()
  
  Function Description:
  
      Returns the First data in the linked list.  This primes the
      current pointer so that next time the Next method is used to get
      more linked list data.
  
  Arguments:
  
      data -- The data from the linked list.
  
  Return Value:
  
      TRUE -- If data is valid.

      FALSE -- If data is not valid.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::First(T &Data)
{
    Current = NIDeque->Root;
    if(Current != NULL){
        Data = Current->Data;
        return TRUE;
    }else{
        return FALSE;
    }
} // End of NIDeque_c::First




/*++
  
  NIDequeIter_c::Next()
  
  Function Description:
  
      Returns the Next data in the linked list.
  
  Arguments:
  
      data -- The data from the linked list.
  
  Return Value:
  
      TRUE -- If data is valid.

      FALSE -- If data is not valid.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::Next(T &Data)
{
    if(Current){
        Current = Current->Next;
		if(Current){
            Data = Current->Data;
            return TRUE;
	    }
    } 
    Data = NULL;              
    return FALSE;
} // End of NIDeque_c::Next




/*++
  
  NIDequeIter_c::Last()
  
  Function Description:
  
      Returns the Last data in the linked list.
  
  Arguments:
  
      data -- The data from the linked list.
  
  Return Value:
  
      TRUE -- If data is valid.

      FALSE -- If data is not valid.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::Last(T &Data)
{
    Current = NIDeque->Tail;
    if(Current != NULL){
        Data = Current->Data;
        return TRUE;
    }else{
        return FALSE;
    }
} // End of NIDeque_c::First




/*++
  
  NIDequeIter_c::Back()
  
  Function Description:
  
      Returns the previous data in the linked list.
  
  Arguments:
  
      data -- The data from the linked list.
  
  Return Value:
  
      TRUE -- If data is valid.

      FALSE -- If data is not valid.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::Back(T &Data)
{
    if(Current && (Current = Current->Back)){
        Data = Current->Data;
        return TRUE;
    }else{ 
        Data = NULL;              
        return FALSE;
    }
} // End of NIDeque_c::Next




/*++
  
  NIDequeIter_c::Replace()
  
  Function Description:
  
      Replaces src by chg.
  
  Arguments:
  
      src -- Data to search for.

      chg -- The new data.
  
  Return Value:
  
      TRUE -- If data is valid.

      FALSE -- If data is not valid.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::Replace(T &src,T chg)
{
    NILNode_c<T> *cptr    = NULL; 
    
    if(Current != NULL){
        src = Current->Data;
        Current->Data = chg;
        return TRUE;
    }                
    return FALSE;
} // End of NIDeque_c::Replace




/*++
  
  NIDequeIter_c::Remove()
  
  Function Description:
  
      Removes whatever the current pointer is pointing at.
  
  Arguments:
  
      data -- Data removed.
  
  Return Value:
  
      TRUE -- If data is valid.

      FALSE -- If data is not valid.
  
--*/
template<class T> BOOL NIDequeIter_c<T>::Remove(T &data)
{            
    NILNode_c<T>    *cptr   = NULL;
    T               *vdata  = NULL;
                            
    if(Current != NULL){
        RemoveData(Current,data);
        return TRUE;
    }                           
    return FALSE;
} // End of NIDeque_c<T>::Remove

#endif
