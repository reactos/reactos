// ArgumentParser.h: interface for the CArgumentParser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(ARGUMENTPARSER_H__D4C1F637_BEBF_11D3_91EE_204C4F4F5020__INCLUDED_)
#define ARGUMENTPARSER_H__D4C1F637_BEBF_11D3_91EE_204C4F4F5020__INCLUDED_

// Use this class to create parser of command line object
class CArgumentParser
{
public:
	// Call this function to specify buffer containing the command line to be parsed
	// Parameters:
	//	pchArguments - pointer to buffer containing the command line. This buffer is modified by object,
	//					and must not be accessed extrenaly when object is used, unless you interate it
	//					only once and modify only substrings returned by GetNextArgument.
	//
	// Remarks:
	// This object can be reused by setting the buffer multiple times.
	void SetArgumentList(TCHAR *pchArguments);

	// Call this function to reset argument iteration. You don't need to call this function after call
	// to set SetArgumentList, because calling SetArgumentList resets iteration with new buffer.
	void ResetArgumentIteration();

	// Call this function to get next argument from command line.
	//
	// Returns:
	//	Function returns next argument. If this is first call after calling SetArgumentList or
	//	ResetArgumentIteration, functions returns the first argument (The command itself ?).
	TCHAR * GetNextArgument();
	CArgumentParser();
	virtual ~CArgumentParser();
private:
	TCHAR *m_pchArgumentList;		// points to begin of argumet list
	const TCHAR *m_pchArgumentListEnd;	// points to last 0 in argument list
	TCHAR *m_pchArgument;
};

#endif // !defined(ARGUMENTPARSER_H__D4C1F637_BEBF_11D3_91EE_204C4F4F5020__INCLUDED_)
