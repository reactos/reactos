#ifndef MemLink_h
#define MemLink_h

typedef icXYZNumber MyXYZNumber;

struct LHTextType {
    OSType							base;				/* 0x74657874 */
    unsigned long					reserved;					/* fill with 0x00 */
    unsigned char					text[1];					/* count of text is obtained from tag size element */
};
typedef struct LHTextType LHTextType;
struct LHTextDescriptionType {
    OSType							typeDescriptor;				/* 0x64657363 */
    unsigned long					reserved;					/* fill with 0x00 */
    unsigned long					ASCIICount;					/* the count of "bytes" */
    unsigned char					ASCIIName[2];				/* Variable size, to access fields after this one, have to count bytes */
    unsigned long					UniCodeCode;
    unsigned long					UniCodeCount;				/* the count of characters, each character has two bytes */
    unsigned char					UniCodeName[2];				/* Variable size */
    short							ScriptCodeCode;
    unsigned char					ScriptCodeCount;			/* the count of "bytes" */
    unsigned char					ScriptCodeName[2];			/* Variable size */
};
typedef struct LHTextDescriptionType LHTextDescriptionType;
typedef struct{
	double X;double Y;double Z;
}MyDoubleXYZ;

CMError MyGetColorSpaces(	CMConcatProfileSet	*profileSet,
							UINT32				*sCS,
							UINT32				*dCS );
#define Round(a) (((a)>0.)?((a)+.5):((a)-.5))
void  MakeMyDoubleXYZ( MyXYZNumber *x, MyDoubleXYZ *ret );
CMError MyAdd_NL_Header( UINT32 theSize, icHeader	*linkHeader,
						 UINT32 aIntent, UINT32 aClass, UINT32 aColorSpace, UINT32 aConnectionSpace );
CMError MyAdd_NL_DescriptionTag	( LHTextDescriptionType *descPtr, unsigned char *theText );
CMError MyAdd_NL_ColorantTag	( icXYZType *descPtr, MyXYZNumber *aColor );
CMError MyAdd_NL_CurveTag	( icCurveType *descPtr, unsigned short Gamma );
CMError MyAdd_NL_CopyrightTag		( unsigned char *copyrightText, LHTextType *aLHTextType );
CMError MyAdd_NL_SequenceDescTag(	CMConcatProfileSet			*profileSet,
						  			icProfileSequenceDescType	*pSeqPtr,
						  			long						*aSize );
CMError MyAdd_NL_AToB0Tag_mft1( CMMModelPtr cw, icLut8Type *lutPtr, long colorLutSize );
CMError MyAdd_NL_AToB0Tag_mft2( CMMModelPtr cw, icLut16Type *lutPtr, long colorLutSize );
CMError MyAdd_NL_HeaderMS	( UINT32 theSize, icHeader	*linkHeader, unsigned long aIntent, icColorSpaceSignature sCS, icColorSpaceSignature dCS );

CMError DeviceLinkFill(	CMMModelPtr cw, 
						CMConcatProfileSet *profileSet, 
						icProfile **theProf,
						unsigned long aIntent );
#endif
