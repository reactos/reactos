#ifndef _OID_HPP
#define _OID_HPP

class Oid
{
	// DESCRIPTION:
	//     array containing the numeric components of the OID
	CWordArray m_nOidComp;

	// DESCRIPTION:
	//     array of pointers to strings, containing symbolic names
	//     for OID's components.
	CObArray  m_szOidComp;

public:
	// DESCRIPTION:
	//     constructor
	Oid();

	// DESCRIPTION:
	//     Adds a new Oid component to the END of the internal arrays!
	// PARAMETERS:
	//     (in) integer component of the Oid
	//     (out) symbolic name of the component
	// RETURN VALUE:
	//      0 on success, -1 on failure
	int AddComponent(int nOidComp, const char * szOidComp);

	// DESCRIPTION:
	//      Reverses the components of the OID from both
	//		m_nOidComp and m_szOidComp
	// RETURN VALUE:
	//      0 on success, -1 on failure
	int ReverseComponents();

	// DESCRIPTION:
	//      Output operator, displays the whole Oid
	friend ostream& operator<< (ostream& outStream, const Oid& oid);

	// DESCRIPTION:
	//     destructor
	~Oid();
};

#endif
