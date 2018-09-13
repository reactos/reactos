
#define MAX_WORD_LENGTH	64						  
						  
typedef struct tagWORD_ATOM {
	ATOM atom;
	DWORD dwHits;
} WORD_ATOM, *PWORD_ATOM;


class CStringBuf {
public:
	CStringBuf(int cGrowBy = 1024);
	~CStringBuf();

	char*	Buf(void)	{ return m_buf; }
	int		Size(void)	{ return m_pos; }
	void	SetSize(int cNew);
	void	AddChar(char c);
	BOOL	AddSZ(char* sz);
	char*	DetachBuf(void);

private:
	int		Grow(int cAdd);
	BOOL	GrowAtomTable(int cAdd);

	// old string buffer

	char*	m_buf;
	int		m_len;
	int		m_pos;
	int		m_growBy;

	// new atom table

	PWORD_ATOM	m_atomTable;
	int			m_nAtoms;
	int			m_currentAtom;
	int			m_bufSize;
};

