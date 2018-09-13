/****************************************************************************\
 *
 *   ARRAY.H ---Class declaration for the array structure
 *	 
\****************************************************************************/

#ifndef _ARRAY_H
#define _ARRAY_H

/*Interface-------------------------------------------------------------------*/
template <class T>
class array {
    private:
        int nLen, nMax;
        T   *pData;
        void Destruct();
    public:
        array();
        ~array();

        BOOL Append(T v);
        int  Length() const;
        void ClearAll();
        void DeleteAll();

        T& operator[](int index);
};

/*definitions of everything*/

#ifndef ARRAY_CXX
#define ARRAY_CXX

/*Implementation------------------------------------------------------------*/
template <class T>
array<T>::array(){
    nLen  = nMax = 0;
    pData = NULL;
}

template <class T>
inline array<T>::~array() {
    if (pData) ::MemFree(pData);
    pData = NULL;
    nMax  = nLen = 0;
}

template <class T>
inline int array<T>::Length() const{
    return nLen;
}

template <class T>
inline T& array<T>::operator[](int index){
    ASSERT(index<Length());
    ASSERT(index>=0);
    ASSERT(pData);
    return pData[index];
}

template <class T>
BOOL array<T>::Append(T v) {
    if (nLen == nMax){
        nMax  = nMax + 8;			/* grow by bigger chunks */
		T* pNew = (T*)::MemReAlloc(pData, sizeof(T)*nMax);
		if (pNew == NULL)
			return FALSE;
        pData = pNew;
    }
    ASSERT(pData);
    ASSERT(nMax);
    pData[nLen++] = v;
	return TRUE;
}

template <class T>
void array<T>::Destruct(){
    while (nLen){
        delete pData[--nLen];
    }
}

template <class T>
inline void array<T>::ClearAll() {
    nLen = 0;
}

template <class T>
inline void array<T>::DeleteAll() {
    Destruct();
}

#endif 
/* ARRAY_CXX */


#endif 
/* _ARRAY_H */


