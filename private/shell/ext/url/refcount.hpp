/*
 * refcount.hpp - RefCount class description.
 */


/* Types
 ********/

/* Classes
 **********/

class RefCount
{
private:
   ULONG m_ulcRef;

public:
   RefCount(void);
   // Virtual destructor defers to destructor of derived class.
   virtual ~RefCount(void);

   // IUnknown methods

   ULONG STDMETHODCALLTYPE AddRef(void);
   ULONG STDMETHODCALLTYPE Release(void);

   // friends

#ifdef DEBUG

   friend BOOL IsValidPCRefCount(const RefCount *pcrefcnt);

#endif

};
DECLARE_STANDARD_TYPES(RefCount);

