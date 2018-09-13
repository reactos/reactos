
// Prototypes for DDE utiltity functions-- most with AW variants.
//


extern HDDEDATA GetTopicListA(
    HCONV   hConv,
    BOOL    fAllTopicsReq);



extern HDDEDATA GetTopicListW(
    HCONV   hConv,
    BOOL    fAllTopicsReq);

extern HDDEDATA GetFormatListA(
    HCONV   hConv,
    HSZ     hszTopic);


extern HDDEDATA GetFormatListW(
    HCONV   hConv,
    HSZ     hszTopic);


#ifdef UNICODE
    #define GetTopicList    GetTopicListA
    #define GetFormatList   GetFormatListA
#else
    #define GetTopicList    GetTopicListW
    #define GetFormatList   GetFormatListW
#endif
