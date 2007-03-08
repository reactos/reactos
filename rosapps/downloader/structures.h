
struct Application
{
	WCHAR Name[0x100];
	WCHAR RegName[0x100];
	WCHAR Version[0x100];
	WCHAR Maintainer[0x100];
	WCHAR Licence[0x100];
	WCHAR Description[0x400];
	WCHAR Location[0x100];
	struct Application* Next;
	struct ScriptElement* InstallScript;
	struct ScriptElement* UninstallScript;
};

struct Category
{
	WCHAR Name[0x100];
	//WCHAR Description[0x100];
	int Icon;
	HANDLE TreeviewItem;
	struct Application* Apps;
	struct Category* Next;
	struct Category* Children;
	struct Category* Parent;
};

struct ScriptElement
{
	WCHAR Func[0x100];
	WCHAR Arg[2][0x100];
	struct ScriptElement* Next;
};

struct lParamDownload
{
	HWND Dlg;
	WCHAR* URL;
	WCHAR* File;
};
