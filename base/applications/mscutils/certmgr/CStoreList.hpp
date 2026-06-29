/*
 * PROJECT:     ReactOS Certificate Manager
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     CStoreList definition
 * COPYRIGHT:   Copyright 2025 Mark Jansen <mark.jansen@reactos.org>
 */

enum class StoreType
{
    User,
    Service,
    Computer,
};

class CStoreList
{
  private:
    CAtlList<CStore *> m_Stores;
    StoreType m_Type;

    static BOOL CALLBACK
    s_StoreCallback(
        const void *pvSystemStore,
        DWORD dwFlags,
        PCERT_SYSTEM_STORE_INFO pStoreInfo,
        void *pvReserved,
        void *pvArg);

  public:
    explicit CStoreList(StoreType type);

    void
    LoadStores();
    DWORD
    StoreTypeFlags() const;

    template <typename Fn>
    void
    ForEach(Fn callback)
    {
        if (m_Stores.IsEmpty())
            LoadStores();

        for (POSITION it = m_Stores.GetHeadPosition(); it; m_Stores.GetNext(it))
        {
            CStore *current = m_Stores.GetAt(it);

            callback(current);
        }
    }
};
