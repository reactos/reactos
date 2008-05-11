
typedef struct tagVFW_FILTERLIST
{
  UINT  cFilters;
  CLSID aClsId[1];
} VFW_FILTERLIST;

#define CFSTR_VFW_FILTERLIST "Video for Windows 4 Filters"
