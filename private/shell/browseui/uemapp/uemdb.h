//***   CEMDBLog --
//

class CEMDBLog : public CRegStrFS
{
public:
    HRESULT CountIncr(char *cmd);

protected:
    CEMDBLog();
    virtual ~CEMDBLog();
    friend CEMDBLog *CEMDBLog_Create(HKEY hk, DWORD grfMode);

private:
};
