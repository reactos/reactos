#ifndef DIALOG_H_INCLUDED
#define DIALOG_H_INCLUDED

class CDialogBase
{
protected:
    virtual INT_PTR DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

public:
    virtual ~CDialogBase() {}
};

class CPropertyPage: public CDialogBase
{
public:
    void SetPropSheetPageMembers(PROPSHEETPAGE* ppsp)
    {
        ppsp->lParam = (LPARAM) this;
        ppsp->pfnDlgProc = CPropertyPage::StaticProc;
    }

private:
    static INT_PTR StaticProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CDialog: public CDialogBase
{
public:
    INT_PTR DoModal(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent)
    {return DialogBoxParam(hInstance, lpTemplate, hWndParent, CDialog::StaticProc, (LPARAM) this);}

private:
    static INT_PTR StaticProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

#endif //!DIALOG_H_INCLUDED
