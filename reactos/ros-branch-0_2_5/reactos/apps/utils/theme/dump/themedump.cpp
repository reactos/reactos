#include <cstdlib>
#include <cstdio>
#include <cwchar>
#include <iostream>
#include <exception>
#include <algorithm>
#include <functional>
#include <list>
#include <string>
#include <map>

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <tmschema.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#define SCHEMA_STRINGS
#define TMT_ENUMDEF (0x0D) // undocumented
#define TMT_ENUMVAL (0x0E) // undocumented
#define TMT_ENUM    (0x0F) // undocumented
#include <tmschema.h>

namespace tmdump
{
 typedef std::map<int, std::wstring> tm_enum_t;
 typedef std::map<std::wstring, tm_enum_t> tm_enums_t;

 struct tm_property_t
 {
  std::wstring name;
  int id;
  int type;
  tm_enums_t::iterator enum_type;
 };

 static tm_enums_t tm_enums;

 typedef std::list<tm_property_t> tm_properties_t;

 static tm_properties_t tm_properties;

 typedef std::map<int, std::wstring> tm_states_t;
 typedef std::map<std::wstring, tm_states_t> tm_state_enums_t;

 static tm_state_enums_t tm_state_enums;

 struct tm_part_t
 {
  std::wstring name;
  tm_state_enums_t::iterator states;
 };

 typedef std::map<int, tm_part_t> tm_parts_t;

 typedef std::map<std::wstring, tm_parts_t> tm_classes_t;

 static tm_classes_t tm_classes;

 class print
 {
  protected:
   std::FILE * m_out;

  public:
   print(std::FILE * out_): m_out(out_) { }
 };

 class property_print:
  public print,
  public std::unary_function<tm_property_t, void>
 {
  private:
   wchar_t const * type_name(const tm_property_t& property_)
   {
    switch(property_.type)
    {
     case TMT_STRING: return L"LPWSTR";
     case TMT_INT: return L"int";
     case TMT_BOOL: return L"BOOL";
     case TMT_COLOR: return L"COLORREF";
     case TMT_MARGINS: return L"MARGINS";
     case TMT_FILENAME: return L"LPWSTR";
     case TMT_SIZE: return L"SIZE";
     case TMT_POSITION: return L"POINT";
     case TMT_RECT: return L"RECT";
     case TMT_FONT: return L"LOGFONT";
     case TMT_INTLIST: return L"INTLIST";
     case TMT_ENUM: return property_.enum_type->first.c_str();
     default: return L"<unknown>";
    }
   }

  public:
   property_print(std::FILE * out_): print(out_) { }

   void operator()(const tm_property_t& property_)
   {
    std::fwprintf
    (
     m_out,
     L"%s\t%s\t%d\n",
     type_name(property_),
     property_.name.c_str(),
     property_.id
    );
   }
 };

 class part_print:
  public print,
  public std::unary_function<tm_parts_t::value_type, void>
 {
  public:
   part_print(std::FILE * out_): print(out_) { }

   void operator()(const tm_parts_t::value_type& part_)
   {
    std::fwprintf
    (
     m_out,
     L"\t%s\t%d\t%s\n",
     part_.second.name.c_str(),
     part_.first,
      part_.second.states == tm_state_enums.end() ?
       L"-" :
       part_.second.states->first.c_str()
    );
   }
 };

 class class_print:
  public print,
  public std::unary_function<tm_classes_t::value_type, void>
 {
  public:
   class_print(std::FILE * out_): print(out_) { }

   void operator()(const tm_classes_t::value_type& class_)
   {
    std::fwprintf(m_out, L"%s\n", class_.first.c_str());

    std::for_each
    (
     class_.second.begin(),
     class_.second.end(),
     part_print(m_out)
    );
   }
 };

 class schema_scan: public std::unary_function<struct TMPROPINFO, void>
 {
  private:
   enum
   {
    at_none,
    at_enum,
    at_parts,
    at_states
   }
   m_state;

   union
   {
    void * p_none;
    tm_enum_t * p_enum;
    tm_parts_t * p_class;
    tm_states_t * p_states;
   }
   m_cur;

   bool has_suffix(const std::wstring& name_, const std::wstring& suffix_)
   {
    if(name_.length() <= suffix_.length()) return false;

    return std::equal
    (
     name_.end() - suffix_.length(),
     name_.end(),
     suffix_.begin()
    );
   }

  public:
   schema_scan(): m_state(at_none) { }

   void operator()(const struct TMPROPINFO& prop_)
   {
    static const std::wstring parts_suffix(L"PARTS");
    static const std::wstring states_suffix(L"STATES");

    std::wstring name(prop_.pszName);

    // Compound declaration
    if(prop_.sEnumVal == TMT_ENUMDEF && prop_.bPrimVal == TMT_ENUMDEF)
    {
     // class
     if(has_suffix(name, parts_suffix))
     {
      m_state = at_parts;
      m_cur.p_class = &
      (
       tm_classes
       [
        std::wstring(name.begin(), name.end() - parts_suffix.length())
       ] = tm_parts_t()
      );
     }
     // states
     else if(has_suffix(name, states_suffix))
     {
      m_state = at_states;
      m_cur.p_states = &
      (
       tm_state_enums
       [
        std::wstring(name.begin(), name.end() - states_suffix.length())
       ] = tm_states_t()
      );
     }
     // enumeration
     else
     {
      m_state = at_enum;      
      m_cur.p_enum = &(tm_enums[name] = tm_enum_t());
     }
    }
    // Enumeration member
    else if(prop_.bPrimVal == TMT_ENUMVAL) switch(m_state)
    {
     // enumeration member
     case at_enum:
     {
      (*m_cur.p_enum)[prop_.sEnumVal] = name;
      break;
     }

     // class part
     case at_parts:
     {
      tm_part_t part;
      part.name = name;
      part.states = tm_state_enums.end();
      (*m_cur.p_class)[prop_.sEnumVal] = part;
      break;
     }

     // state
     case at_states:
     {
      (*m_cur.p_states)[prop_.sEnumVal] = name;
      break;
     }
    }
    // Primitive type
    else if(prop_.sEnumVal == prop_.bPrimVal)
    {
     m_state = at_none;
     m_cur.p_none = NULL;
    }
    // Property
    else
    {
     m_state = at_none;
     m_cur.p_none = NULL;

     tm_property_t property;

     property.name = name;
     property.id = prop_.sEnumVal;
     property.type = prop_.bPrimVal;

     if(prop_.bPrimVal == TMT_ENUM)
      property.enum_type = tm_enums.find(name);

     tm_properties.push_back(property);
    }
   }
 };

 struct state_mapping_t
 {
  LPWSTR classname;
  int partid;
  LPWSTR states;
 };

 static const tmdump::state_mapping_t state_map[] =
 {
  { L"BUTTON", 0, NULL },
   { NULL, BP_CHECKBOX, L"CHECKBOX" },
   { NULL, BP_GROUPBOX, L"GROUPBOX" },
   { NULL, BP_PUSHBUTTON, L"PUSHBUTTON" },
   { NULL, BP_RADIOBUTTON, L"RADIOBUTTON" },
  { L"CLOCK", CLP_TIME, L"CLOCK" },
  { L"COMBOBOX", CP_DROPDOWNBUTTON, L"COMBOBOX" },
  { L"EDIT", EP_EDITTEXT, L"EDITTEXT" },
  { L"EXPLORERBAR", 0, NULL },
   { NULL, EBP_HEADERCLOSE, L"HEADERCLOSE" },
   { NULL, EBP_HEADERPIN, L"HEADERPIN" },
   { NULL, EBP_IEBARMENU, L"IEBARMENU" },
   { NULL, EBP_NORMALGROUPCOLLAPSE, L"NORMALGROUPCOLLAPSE" },
   { NULL, EBP_NORMALGROUPEXPAND, L"NORMALGROUPEXPAND" },
   { NULL, EBP_SPECIALGROUPCOLLAPSE, L"SPECIALGROUPCOLLAPSE"},
   { NULL, EBP_SPECIALGROUPEXPAND, L"SPECIALGROUPEXPAND" },
  { L"HEADER", 0, NULL },
   { NULL, HP_HEADERITEM, L"HEADERITEM" },
   { NULL, HP_HEADERITEMLEFT, L"HEADERITEMLEFT" },
   { NULL, HP_HEADERITEMRIGHT, L"HEADERITEMRIGHT" },
   { NULL, HP_HEADERSORTARROW, L"HEADERSORTARROW" },
  { L"LISTVIEW", LVP_LISTITEM, L"LISTITEM " },
  { L"MENU", 0, NULL },
   { NULL, MP_MENUBARDROPDOWN, L"MENU" },
   { NULL, MP_MENUBARITEM, L"MENU" },
   { NULL, MP_CHEVRON, L"MENU" },
   { NULL, MP_MENUDROPDOWN, L"MENU" },
   { NULL, MP_MENUITEM, L"MENU" },
   { NULL, MP_SEPARATOR, L"MENU" },
  { L"MENUBAND", MDP_NEWAPPBUTTON, L"MENUBAND" },
  { L"PAGE", 0, NULL },
   { NULL, PGRP_DOWN, L"DOWN" },
   { NULL, PGRP_DOWNHORZ, L"DOWNHORZ" },
   { NULL, PGRP_UP, L"UP" },
   { NULL, PGRP_UPHORZ, L"UPHORZ" },
  { L"REBAR", RP_CHEVRON, L"CHEVRON" },
  { L"SCROLLBAR", 0, NULL },
   { NULL, SBP_ARROWBTN, L"ARROWBTN" },
   { NULL, SBP_LOWERTRACKHORZ, L"SCROLLBAR" },
   { NULL, SBP_LOWERTRACKVERT, L"SCROLLBAR" },
   { NULL, SBP_THUMBBTNHORZ, L"SCROLLBAR" },
   { NULL, SBP_THUMBBTNVERT, L"SCROLLBAR" },
   { NULL, SBP_UPPERTRACKHORZ, L"SCROLLBAR" },
   { NULL, SBP_UPPERTRACKVERT, L"SCROLLBAR" },
   { NULL, SBP_SIZEBOX, L"SIZEBOX" },
  { L"SPIN", 0, NULL },
   { NULL, SPNP_DOWN, L"DOWN" },
   { NULL, SPNP_DOWNHORZ, L"DOWNHORZ" },
   { NULL, SPNP_UP, L"UP" },
   { NULL, SPNP_UPHORZ, L"UPHORZ" },
  { L"STARTPANEL", 0, NULL },
   { NULL, SPP_LOGOFFBUTTONS, L"LOGOFFBUTTONS" },
   { NULL, SPP_MOREPROGRAMSARROW, L"MOREPROGRAMSARROW" },
  { L"TAB", 0, NULL },
   { NULL, TABP_TABITEM, L"TABITEM" },
   { NULL, TABP_TABITEMBOTHEDGE, L"TABITEMBOTHEDGE" },
   { NULL, TABP_TABITEMLEFTEDGE, L"TABITEMLEFTEDGE" },
   { NULL, TABP_TABITEMRIGHTEDGE, L"TABITEMRIGHTEDGE" },
   { NULL, TABP_TOPTABITEM, L"TOPTABITEM" },
   { NULL, TABP_TOPTABITEMBOTHEDGE, L"TOPTABITEMBOTHEDGE" },
   { NULL, TABP_TOPTABITEMLEFTEDGE, L"TOPTABITEMLEFTEDGE" },
   { NULL, TABP_TOPTABITEMRIGHTEDGE, L"TOPTABITEMRIGHTEDGE" },
  { L"TOOLBAR", 0, NULL },
   { NULL, TP_BUTTON, L"TOOLBAR" },
   { NULL, TP_DROPDOWNBUTTON, L"TOOLBAR" },
   { NULL, TP_SPLITBUTTON, L"TOOLBAR" },
   { NULL, TP_SPLITBUTTONDROPDOWN, L"TOOLBAR" },
   { NULL, TP_SEPARATOR, L"TOOLBAR" },
   { NULL, TP_SEPARATORVERT, L"TOOLBAR" },
  { L"TOOLTIP", 0, NULL },
   { NULL, TTP_BALLOON, L"BALLOON" },
   { NULL, TTP_BALLOONTITLE, L"BALLOON" },
   { NULL, TTP_CLOSE, L"CLOSE" },
   { NULL, TTP_STANDARD, L"STANDARD" },
   { NULL, TTP_STANDARDTITLE, L"STANDARD" },
  { L"TRACKBAR", 0, NULL },
   { NULL, TKP_THUMB, L"THUMB" },
   { NULL, TKP_THUMBBOTTOM, L"THUMBBOTTOM" },
   { NULL, TKP_THUMBLEFT, L"THUMBLEFT" },
   { NULL, TKP_THUMBRIGHT, L"THUMBRIGHT" },
   { NULL, TKP_THUMBTOP, L"THUMBTOP" },
   { NULL, TKP_THUMBVERT, L"THUMBVERT" },
   { NULL, TKP_TICS, L"TICS" },
   { NULL, TKP_TICSVERT, L"TICSVERT" },
   { NULL, TKP_TRACK, L"TRACK" },
   { NULL, TKP_TRACKVERT, L"TRACKVERT" },
  { L"TREEVIEW", 0, NULL },
   { NULL, TVP_GLYPH, L"GLYPH" },
   { NULL, TVP_TREEITEM, L"TREEITEM" },
  { L"WINDOW", 0, NULL },
   { NULL, WP_CAPTION, L"CAPTION" },
   { NULL, WP_CLOSEBUTTON, L"CLOSEBUTTON" },
   { NULL, WP_FRAMEBOTTOM, L"FRAME" },
   { NULL, WP_FRAMELEFT, L"FRAME" },
   { NULL, WP_FRAMERIGHT, L"FRAME" },
   { NULL, WP_HELPBUTTON, L"HELPBUTTON" },
   { NULL, WP_HORZSCROLL, L"HORZSCROLL" },
   { NULL, WP_HORZTHUMB, L"HORZTHUMB" },
   { NULL, WP_MAXBUTTON, L"MAXBUTTON" },
   { NULL, WP_MAXCAPTION, L"MAXCAPTION" },
   { NULL, WP_MDICLOSEBUTTON, L"CLOSEBUTTON" },
   { NULL, WP_MDIHELPBUTTON, L"HELPBUTTON" },
   { NULL, WP_MDIMINBUTTON, L"MINBUTTON" },
   { NULL, WP_MDIRESTOREBUTTON, L"RESTOREBUTTON" },
   { NULL, WP_MDISYSBUTTON, L"SYSBUTTON" },
   { NULL, WP_MINBUTTON, L"MINBUTTON" },
   { NULL, WP_MINCAPTION, L"MINCAPTION" },
   { NULL, WP_RESTOREBUTTON, L"RESTOREBUTTON" },
   { NULL, WP_SMALLCAPTION, L"CAPTION" },
   { NULL, WP_SMALLCLOSEBUTTON, L"CLOSEBUTTON" },
   { NULL, WP_SMALLFRAMEBOTTOM, L"FRAME" },
   { NULL, WP_SMALLFRAMELEFT, L"FRAME" },
   { NULL, WP_SMALLFRAMERIGHT, L"FRAME" },
   { NULL, WP_SMALLMAXCAPTION, L"MAXCAPTION" },
   { NULL, WP_SMALLMINCAPTION, L"MINCAPTION" },
   { NULL, WP_SYSBUTTON, L"SYSBUTTON" },
   { NULL, WP_VERTSCROLL, L"HORZSCROLL" },
   { NULL, WP_VERTTHUMB, L"HORZTHUMB" },
 };

 class state_link: public std::unary_function<struct state_mapping_t, void>
 {
  private:
   tm_classes_t::iterator m_class;

  public:
   void operator()(const struct state_mapping_t& mapping_)
   {
    // switch to a new class
    if(mapping_.classname)
     m_class = tm_classes.find(std::wstring(mapping_.classname));

    // no mapping, or class not found
    if(mapping_.states == NULL || m_class == tm_classes.end()) return;

    tm_state_enums_t::iterator states =
     tm_state_enums.find(std::wstring(mapping_.states));

    // unknown set of states
    if(states == tm_state_enums.end()) return;

    tm_parts_t::iterator part = m_class->second.find(mapping_.partid);

    // unknown part
    if(part == m_class->second.end()) return;

    // success
    part->second.states = states;
   }
 };
}

int main(int argc, char * argv[])
{
 try
 {
  struct TMSCHEMAINFO const & schema = *GetSchemaInfo();

  // build the tables of properties, classes, parts and states
  std::for_each
  (
   schema.pPropTable,
   schema.pPropTable + schema.iPropCount,
   tmdump::schema_scan()
  );

  // link parts to states
  std::for_each
  (
   tmdump::state_map,
   tmdump::state_map + sizeof(tmdump::state_map) / sizeof(tmdump::state_map[0]),
   tmdump::state_link()
  );

  ::InitCommonControls();
  ::SetThemeAppProperties(STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS);

  // dump the current values of all properties
  for
  (
   tmdump::tm_classes_t::iterator p = tmdump::tm_classes.begin();
   p != tmdump::tm_classes.end();
   ++ p
  )
  {
   const std::wstring& class_name = p->first;

   // open the theme data for the current class
   class htheme_t
   {
    public:
     HTHEME m_handle;
     
     htheme_t(HTHEME handle_): m_handle(handle_) { }
     ~htheme_t() { ::CloseThemeData(m_handle); }

     operator HTHEME() { return m_handle; }
   }
   data = ::OpenThemeData(NULL, class_name.c_str());

   // failure
   if(data == NULL)
   {
    std::fwprintf
    (
     stderr,
     L"OpenThemeData(\"%s\") failed, error %d\n",
     class_name.c_str(),
     GetLastError()
    );

    continue;
   }

   // class name
   std::fwprintf(stdout, L"%s\n", p->first.c_str());

   // dump system properties
   for
   (
    tmdump::tm_properties_t::iterator p = tmdump::tm_properties.begin();
    p != tmdump::tm_properties.end();
    ++ p
   )
   {
    switch(p->type)
    {
     case TMT_POSITION:
     case TMT_MARGINS:
     case TMT_RECT:
     case TMT_INTLIST:
      continue;
    }

    // property name
    std::fwprintf(stdout, L"\t%s = ", p->name.c_str());

    HRESULT hres;

    switch(p->type)
    {
     // string
     case TMT_STRING:
     case TMT_FILENAME: // FIXME
     {
      WCHAR buffer[256];
      if(FAILED(hres = GetThemeSysString(data, p->id, buffer, 256))) break;
      std::fwprintf(stdout, L"string: \"%s\"", buffer);
      break;
     }

     // integer
     case TMT_INT:
     {
      int val;
      if(FAILED(hres = GetThemeSysInt(data, p->id, &val))) break;
      std::fwprintf(stdout, L"int: %d", val);
      break;
     }

     // boolean
     case TMT_BOOL:
     {
      SetLastError(0);
      BOOL val = GetThemeSysBool(data, p->id);
      if(FAILED(hres = GetLastError())) break;
      std::fwprintf(stdout, L"bool: %s", val ? L"true" : L"false");
      break;
     }

     // color
     case TMT_COLOR:
     {
      SetLastError(0);

      COLORREF val = GetThemeSysColor(data, p->id);

      if(FAILED(hres = GetLastError())) break;

      std::fwprintf
      (
       stdout,
       L"rgb: %d, %d, %d",
       GetRValue(val),
       GetGValue(val),
       GetBValue(val)
      );

      break;
     }

     // size
     case TMT_SIZE:
     {
      SetLastError(0);
      int val = GetThemeSysSize(data, p->id);
      if(FAILED(hres = GetLastError())) break;
      std::fwprintf(stdout, L"size: %d", val);
      break;
     }

     // font
     case TMT_FONT:
     {
      LOGFONTW val;
      if(FAILED(hres = GetThemeSysFont(data, p->id, &val))) break;
      std::fwprintf(stdout, L"font: %s", val.lfFaceName);
      break;
     }

     // enumerated integer
     case TMT_ENUM:
     {
      int val;
      if(FAILED(hres = GetThemeSysInt(data, p->id, &val))) break;

      std::fwprintf(stdout, L"enum(%s): ", p->enum_type->first.c_str());

      tmdump::tm_enum_t::iterator enumval = p->enum_type->second.find(val);

      if(enumval == p->enum_type->second.end())
       std::fwprintf(stdout, L"<%d>", val);
      else
       std::fwprintf(stdout, L"%s", enumval->second.c_str());

      break;
     }
    }

    if(FAILED(hres)) std::fwprintf(stdout, L"<error %08X>", hres);

    std::fputwc(L'\n', stdout);
   }

  }
 }
 catch(std::exception e)
 {
  std::cerr << e.what() << std::endl;
  return EXIT_FAILURE;
 }

 return EXIT_SUCCESS;
}

// EOF

