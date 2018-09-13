//
// infdata.h
//
#ifdef DECLARE_DATA
SHORT           g_StaticDistanceTreeTable[STATIC_BLOCK_DISTANCE_TABLE_SIZE];
SHORT           g_StaticLiteralTreeTable[STATIC_BLOCK_LITERAL_TABLE_SIZE];
#else
extern SHORT    g_StaticDistanceTreeTable[STATIC_BLOCK_DISTANCE_TABLE_SIZE];
extern SHORT    g_StaticLiteralTreeTable[STATIC_BLOCK_LITERAL_TABLE_SIZE];
#endif


