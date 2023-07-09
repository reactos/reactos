template<class T, const DWORD m_dwRegType>
DWORD
CItem_WKSP<T, m_dwRegType>::
CalcSizeOfData()
const
{
    switch (GetRegType()) {
    default:
        Assert(!"Unsupported data type.");
        return 0;

    case REG_BINARY:
    case REG_DWORD:
        // No side effects
        Assert(GetPtrToData());

        return sizeof(T);
        break;

    case REG_SZ:
        {
            PSTR psz = (PSTR) (*(void * *)GetPtrToData());

            return WKSP_StrSize(psz);
        }
        break;

    case REG_MULTI_SZ:
        {
            PSTR psz = (PSTR) (*(void * *)GetPtrToData());

            return WKSP_MultiStrSize(psz);
        }
        break;
    }
}

