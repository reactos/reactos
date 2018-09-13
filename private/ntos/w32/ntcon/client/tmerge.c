/*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*/

#include "precomp.h"
#pragma hdrstop
#pragma hdrstop

// run-length encoded data structure for attributes

typedef struct _ATTR_PAIR {
    WORD Length;            // number of attribute appears
    WORD Attr;              // attribute
} ATTR_PAIR, *PATTR_PAIR;


#define SOURCE1_LENGTH 4
#define MERGE1_LENGTH 1
#define TARGET1_LENGTH 4
ATTR_PAIR Source1[SOURCE1_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
ATTR_PAIR Merge1[MERGE1_LENGTH] = { 2, 20 };

ATTR_PAIR Target1[TARGET1_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
#define START_INDEX1 4
#define END_INDEX1 5

//******************************************

#define SOURCE2_LENGTH 4
#define MERGE2_LENGTH 2
#define TARGET2_LENGTH 6
ATTR_PAIR Source2[SOURCE2_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
ATTR_PAIR Merge2[MERGE2_LENGTH] = { 1, 20,
                                    1, 30
                                  };
ATTR_PAIR Target2[TARGET2_LENGTH] = { 2, 10,
                                      3, 20,
                                      1, 30,
                                      1, 20,
                                      3, 30,
                                      1, 40
                                    };
#define START_INDEX2 4
#define END_INDEX2 5

//******************************************

#define SOURCE3_LENGTH 4
#define MERGE3_LENGTH 2
#define TARGET3_LENGTH 5
ATTR_PAIR Source3[SOURCE3_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
ATTR_PAIR Merge3[MERGE3_LENGTH] = { 1, 20,
                                    1, 30
                                  };
ATTR_PAIR Target3[TARGET3_LENGTH] = { 2, 10,
                                      5, 20,
                                      2, 30,
                                      1, 20,
                                      1, 30
                                    };
#define START_INDEX3 9
#define END_INDEX3 10

//******************************************

#define SOURCE4_LENGTH 4
#define MERGE4_LENGTH 4
#define TARGET4_LENGTH 4
ATTR_PAIR Source4[SOURCE4_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
ATTR_PAIR Merge4[MERGE4_LENGTH] = { 1, 20,
                                    1, 30,
                                    3, 20,
                                    6, 60
                                  };
ATTR_PAIR Target4[TARGET4_LENGTH] = { 1, 20,
                                      1, 30,
                                      3, 20,
                                      6, 60
                                    };
#define START_INDEX4 0
#define END_INDEX4 10

//******************************************

#define SOURCE5_LENGTH 4
#define MERGE5_LENGTH 3
#define TARGET5_LENGTH 6
ATTR_PAIR Source5[SOURCE5_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
ATTR_PAIR Merge5[MERGE5_LENGTH] = { 1, 10,
                                    1, 70,
                                    1, 30
                                  };
ATTR_PAIR Target5[TARGET5_LENGTH] = { 3, 10,
                                      1, 70,
                                      1, 30,
                                      2, 20,
                                      3, 30,
                                      1, 40
                                    };
#define START_INDEX5 2
#define END_INDEX5 4

//******************************************

#define SOURCE6_LENGTH 4
#define MERGE6_LENGTH 3
#define TARGET6_LENGTH 6
ATTR_PAIR Source6[SOURCE6_LENGTH] = { 2, 10,
                                      5, 20,
                                      3, 30,
                                      1, 40
                                    };
ATTR_PAIR Merge6[MERGE6_LENGTH] = { 1, 10,
                                    1, 70,
                                    1, 30
                                  };
ATTR_PAIR Target6[TARGET6_LENGTH] = { 2, 10,
                                      2, 20,
                                      1, 10,
                                      1, 70,
                                      4, 30,
                                      1, 40
                                    };
#define START_INDEX6 4
#define END_INDEX6 6

//******************************************

#define SOURCE7_LENGTH 1
#define MERGE7_LENGTH 2
#define TARGET7_LENGTH 2
ATTR_PAIR Source7[SOURCE7_LENGTH] = { 3, 10
                                    };
ATTR_PAIR Merge7[MERGE7_LENGTH] = { 1, 20,
                                    2, 70
                                  };
ATTR_PAIR Target7[TARGET7_LENGTH] = { 1, 20,
                                      2, 70
                                    };
#define START_INDEX7 0
#define END_INDEX7 2

//******************************************

#define SOURCE8_LENGTH 1
#define MERGE8_LENGTH 2
#define TARGET8_LENGTH 3
ATTR_PAIR Source8[SOURCE8_LENGTH] = { 3, 10
                                    };
ATTR_PAIR Merge8[MERGE8_LENGTH] = { 1, 20,
                                    1, 80
                                  };
ATTR_PAIR Target8[TARGET8_LENGTH] = { 1, 20,
                                      1, 80,
                                      1, 10
                                    };
#define START_INDEX8 0
#define END_INDEX8 1

//******************************************

#define SOURCE9_LENGTH 1
#define MERGE9_LENGTH 1
#define TARGET9_LENGTH 2
ATTR_PAIR Source9[SOURCE9_LENGTH] = { 3, 10
                                    };
ATTR_PAIR Merge9[MERGE9_LENGTH] = { 1, 20
                                  };
ATTR_PAIR Target9[TARGET9_LENGTH] = { 1, 20,
                                      2, 10
                                    };
#define START_INDEX9 0
#define END_INDEX9 0


//******************************************

#define SOURCE10_LENGTH 1
#define MERGE10_LENGTH 1
#define TARGET10_LENGTH 3
ATTR_PAIR Source10[SOURCE10_LENGTH] = { 3, 10
                                    };
ATTR_PAIR Merge10[MERGE10_LENGTH] = { 1, 20
                                  };
ATTR_PAIR Target10[TARGET10_LENGTH] = { 1, 10,
                                        1, 20,
                                        1, 10
                                       };
#define START_INDEX10 1
#define END_INDEX10 1


MergeAttrStrings(
    PATTR_PAIR Source,
    WORD SourceLength,
    PATTR_PAIR Merge,
    WORD MergeLength,
    OUT PATTR_PAIR *Target,
    OUT LPWORD TargetLength,
    IN WORD StartIndex,
    IN WORD EndIndex
    )

/*++

--*/
{
    PATTR_PAIR SrcAttr,TargetAttr,SrcEnd;
    PATTR_PAIR NewString;
    WORD NewStringLength=0;
    WORD i;

    NewString = (PATTR_PAIR) LocalAlloc(LMEM_FIXED,(SourceLength+MergeLength+1)*sizeof(ATTR_PAIR));

    //
    // copy the source string, up to the start index.
    //

    SrcAttr = Source;
    SrcEnd = Source + SourceLength;
    TargetAttr = NewString;
    if (StartIndex != 0) {
        for (i=0;i<StartIndex;) {
            i += SrcAttr->Length;
            *TargetAttr++ = *SrcAttr++;
        }

        //
        // back up to the last pair copied, in case the attribute in the first
        // pair in the merge string matches.  also, adjust TargetAttr->Length
        // based on i, the attribute
        // counter, back to the StartIndex.  i will be larger than the
        // StartIndex in the case where the last attribute pair copied had
        // a length greater than the number needed to reach StartIndex.
        //

        TargetAttr--;
        if (i>StartIndex) {
            TargetAttr->Length -= i-StartIndex;
        }
        if (Merge->Attr == TargetAttr->Attr) {
            TargetAttr->Length += Merge->Length;
            MergeLength-=1;
            Merge++;
        }
        TargetAttr++;
    } else {
        i=0;
    }

    //
    // copy the merge string.
    //

    RtlCopyMemory(TargetAttr,Merge,MergeLength*sizeof(ATTR_PAIR));
    TargetAttr += MergeLength;

    //
    // figure out where to resume copying the source string.
    //

    while (i<=EndIndex) {
        i += SrcAttr->Length;
        SrcAttr++;
    }

    //
    // if not done, copy the rest of the source
    //

    if (SrcAttr != SrcEnd || i!=(EndIndex+1)) {

        //
        // see if we've gone past the right attribute.  if so, back up and
        // copy the attribute and the correct length.
        //

        TargetAttr--;
        if (i>(SHORT)(EndIndex+1)) {
            SrcAttr--;
            if (TargetAttr->Attr == SrcAttr->Attr) {
                TargetAttr->Length += i-(EndIndex+1);
            } else {
                TargetAttr++;
                TargetAttr->Attr = SrcAttr->Attr;
                TargetAttr->Length = i-(EndIndex+1);
            }
            SrcAttr++;
        }

        //
        // see if we can merge the source and target.
        //

        else if (TargetAttr->Attr == SrcAttr->Attr) {
            TargetAttr->Length += SrcAttr->Length;
            i += SrcAttr->Length;
            SrcAttr++;
        }
        TargetAttr++;

        //
        // copy the rest of the source
        //

        RtlCopyMemory(TargetAttr,SrcAttr,(SourceLength*sizeof(ATTR_PAIR)) - ((ULONG)SrcAttr - (ULONG)Source));
    }

    TargetAttr = (PATTR_PAIR)((ULONG)TargetAttr + (SourceLength*sizeof(ATTR_PAIR)) - ((ULONG)SrcAttr - (ULONG)Source));
    *TargetLength = (WORD)(TargetAttr - NewString);
    *Target = NewString;
    return;
}

DWORD
main(
    int argc,
    char *argv[]
    )
{
    PATTR_PAIR Target;
    WORD TargetLength;
    WORD i;

    MergeAttrStrings(Source1,
                     SOURCE1_LENGTH,
                     Merge1,
                     MERGE1_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX1,
                     END_INDEX1
                    );
    if (TargetLength != TARGET1_LENGTH) {
        printf("merge 1 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET1_LENGTH);
    }
    if (memcmp(Target,Target1,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target1[i].Attr);
            printf("expected length is %d\n",Target1[i].Length);
        }
    }
    MergeAttrStrings(Source2,
                     SOURCE2_LENGTH,
                     Merge2,
                     MERGE2_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX2,
                     END_INDEX2
                    );
    if (TargetLength != TARGET2_LENGTH) {
        printf("merge 2 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET2_LENGTH);
    }
    if (memcmp(Target,Target2,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target2[i].Attr);
            printf("expected length is %d\n",Target2[i].Length);
        }
    }
    MergeAttrStrings(Source3,
                     SOURCE3_LENGTH,
                     Merge3,
                     MERGE3_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX3,
                     END_INDEX3
                    );
    if (TargetLength != TARGET3_LENGTH) {
        printf("merge 3 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET3_LENGTH);
    }
    if (memcmp(Target,Target3,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target3[i].Attr);
            printf("expected length is %d\n",Target3[i].Length);
        }
    }
    MergeAttrStrings(Source4,
                     SOURCE4_LENGTH,
                     Merge4,
                     MERGE4_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX4,
                     END_INDEX4
                    );
    if (TargetLength != TARGET4_LENGTH) {
        printf("merge 4 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET4_LENGTH);
    }
    if (memcmp(Target,Target4,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target4[i].Attr);
            printf("expected length is %d\n",Target4[i].Length);
        }
    }
    MergeAttrStrings(Source5,
                     SOURCE5_LENGTH,
                     Merge5,
                     MERGE5_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX5,
                     END_INDEX5
                    );
    if (TargetLength != TARGET5_LENGTH) {
        printf("merge 5 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET5_LENGTH);
    }
    if (memcmp(Target,Target5,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target5[i].Attr);
            printf("expected length is %d\n",Target5[i].Length);
        }
    }
    MergeAttrStrings(Source6,
                     SOURCE6_LENGTH,
                     Merge6,
                     MERGE6_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX6,
                     END_INDEX6
                    );
    if (TargetLength != TARGET6_LENGTH) {
        printf("merge 6 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET6_LENGTH);
    }
    if (memcmp(Target,Target6,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target6[i].Attr);
            printf("expected length is %d\n",Target6[i].Length);
        }
    }
    MergeAttrStrings(Source7,
                     SOURCE7_LENGTH,
                     Merge7,
                     MERGE7_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX7,
                     END_INDEX7
                    );
    if (TargetLength != TARGET7_LENGTH) {
        printf("merge 7 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET7_LENGTH);
    }
    if (memcmp(Target,Target7,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target7[i].Attr);
            printf("expected length is %d\n",Target7[i].Length);
        }
    }
    MergeAttrStrings(Source8,
                     SOURCE8_LENGTH,
                     Merge8,
                     MERGE8_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX8,
                     END_INDEX8
                    );
    if (TargetLength != TARGET8_LENGTH) {
        printf("merge 8 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET8_LENGTH);
    }
    if (memcmp(Target,Target8,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target8[i].Attr);
            printf("expected length is %d\n",Target8[i].Length);
        }
    }
    MergeAttrStrings(Source9,
                     SOURCE9_LENGTH,
                     Merge9,
                     MERGE9_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX9,
                     END_INDEX9
                    );
    if (TargetLength != TARGET9_LENGTH) {
        printf("merge 9 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET9_LENGTH);
    }
    if (memcmp(Target,Target9,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target9[i].Attr);
            printf("expected length is %d\n",Target9[i].Length);
        }
    }
    MergeAttrStrings(Source10,
                     SOURCE10_LENGTH,
                     Merge10,
                     MERGE10_LENGTH,
                     &Target,
                     &TargetLength,
                     START_INDEX10,
                     END_INDEX10
                    );
    if (TargetLength != TARGET10_LENGTH) {
        printf("merge 10 failed\n");
        printf("TargetLength is %d.  expected %d\n",TargetLength,TARGET10_LENGTH);
    }
    if (memcmp(Target,Target10,TargetLength)) {
        printf("Target didn't match\n");
        for (i=0;i<TargetLength;i++) {
            printf("returned attr is %d\n",Target[i].Attr);
            printf("returned length is %d\n",Target[i].Length);
            printf("expected attr is %d\n",Target10[i].Attr);
            printf("expected length is %d\n",Target10[i].Length);
        }
    }
}
