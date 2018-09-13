#ifndef _INC_DSKQUOTA_FORMAT_H
#define _INC_DSKQUOTA_FORMAT_H
///////////////////////////////////////////////////////////////////////////////
/*  File: format.h

    Description: Declaration for class EnumFORMATETC.
        Moved from original location in dataobj.h (deleted from project).


    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/28/97    Initial creation.                                    BrianAu
*/
///////////////////////////////////////////////////////////////////////////////

class EnumFORMATETC : public IEnumFORMATETC
{
    private:
        LONG        m_cRef;
        UINT        m_cFormats;
        UINT        m_iCurrent;
        LPFORMATETC m_prgFormats;

        //
        // Prevent assignment.
        //
        void operator = (const EnumFORMATETC&);

    public:
        EnumFORMATETC(UINT cFormats, LPFORMATETC prgFormats);
        EnumFORMATETC(const EnumFORMATETC& ef);
        ~EnumFORMATETC(VOID);

        //
        // IUnknown methods.
        //
        STDMETHODIMP         
        QueryInterface(
            REFIID riid, 
            LPVOID *ppvOut);

        STDMETHODIMP_(ULONG) 
        AddRef(
            VOID);

        STDMETHODIMP_(ULONG) 
        Release(
            VOID);

        //
        // IEnumFORMATETC methods.
        //
        STDMETHODIMP 
        Next(
            DWORD, 
            LPFORMATETC, 
            LPDWORD);

        STDMETHODIMP 
        Skip(
            DWORD);

        STDMETHODIMP 
        Reset(
            VOID);

        STDMETHODIMP 
        Clone(
            IEnumFORMATETC **);
};
        
#endif // _INC_DSKQUOTA_FORMAT_H
