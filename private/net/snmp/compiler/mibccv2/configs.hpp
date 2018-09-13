#ifndef _CONFIGS_HPP
#define _CONFIGS_HPP

#define CFG_PRINT_LOGO		0x01
#define	CFG_PRINT_TREE		0x02
#define CFG_PRINT_NODE		0x04
#define CFG_VERB_ERROR		0x08
#define CFG_VERB_WARNING	0x10

#define CFG_DEF_MAXERRORS	10

class Configs
{
public:
	LPSTR	m_pszOutputFilename;
	int		m_nMaxErrors;
	DWORD	m_dwFlags;

	Configs();
	~Configs();
};

#endif
