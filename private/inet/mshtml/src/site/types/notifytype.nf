//-----------------------------------------------------------------------------
//
//  File: notifications.nf
//
//  Contents: This file contains notification descriptions
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
//  Description:
//      Notification is a named event sent to one or more targets.
//      It is described by its type and an element, a range, or both.
//      There are two types of ranges: character and tree. A character
//      range begins at some absolute cp and extends for some number of
//      characters. A tree range similarly covers one or more elements,
//      expressed using the source index of the first affected element
//      and the number of elements affected. Both ranges may be supplied in
//      a single notification.
//      For efficiency, notifications are categorized and are delivered
//      only to targets which accept the specified category.
//
//      NOTE: If an Element is supplied without a range, the appropriate range
//            information is inferred from the element.
//
//  Specification Format:
//
//      A notification begins with a "type:" and must include one or more
//      "targets:" and at least one category.
//
//      Argument            Values
//
//        type:         notification name (in all CAPS, use underscore to separate words)
//        arguments:    element, si, celements, cp, cch, tree-node, data, flags
//        targets:      self, ancestors, descendents, tree-level
//        categories:   text-change, tree-change, layout-change, activex, all-elements
//        properties:   send-until-handled, lazy-range, super-last
//
//-----------------------------------------------------------------------------


type: CHARS_ADDED;
    arguments: cp, cch, tree-node;
    targets: self, ancestors, tree-level;
    categories: text-change;

type: CHARS_DELETED;
    arguments: cp, cch, tree-node;
    targets: self, ancestors, tree-level;
    categories: text-change;

type: CHARS_RESIZE;
    arguments: cp, cch, tree-node;
    targets: self, ancestors;
    categories: layout-change, layout-elements;
    properties: send-until-handled;

type: CHARS_INVALIDATE;
    arguments: cp, cch, tree-node;
    targets: self, ancestors;
    categories: layout-change, layout-elements, clean-change;
    properties: send-until-handled;

type: ELEMENTS_ADDED;
    arguments: si, celements;
    targets: tree-level;
    categories: tree-change;

type: ELEMENTS_DELETED;
    arguments: si, celements;
    targets: tree-level;
    categories: tree-change;

type: ELEMENT_RESIZE;
    arguments: element, flags;
    targets: ancestors, tree-level;
    categories: layout-change, layout-elements;
    properties: send-until-handled, lazy-range;

type: ELEMENT_REMEASURE;
    arguments: element, flags;
    targets: self, ancestors, tree-level;
    categories: layout-change, layout-elements;
    properties: send-until-handled, lazy-range, clean-change;

type: ELEMENT_RESIZEANDREMEASURE;
    arguments: element, flags;
    targets: ancestors, tree-level;
    categories: layout-change, layout-elements;
    properties: send-until-handled, lazy-range, clean-change;

type: ELEMENT_SIZECHANGED;
    arguments: element, flags;
    targets: tree-level;
    categories: layout-change;
    properties: send-until-handled, lazy-range;

type: ELEMENT_POSITIONCHANGED;
    arguments: element, flags;
    targets: tree-level;
    categories: layout-change;
    properties: send-until-handled, lazy-range;

type: ELEMENT_ADD_ADORNER;
    arguments: element, flags;
    targets: self, ancestors;
    categories: layout-change;
    properties: lazy-range, send-until-handled, clean-change;

type: ELEMENT_ZCHANGE;
    arguments: element, flags;
    targets: ancestors;
    categories: layout-change, layout-elements;
    properties: send-until-handled, lazy-range;

type: ELEMENT_REPOSITION;
    arguments: element, flags;
    targets: ancestors;
    categories: layout-change, layout-elements;
    properties: send-until-handled, lazy-range;

type: RANGE_ENSURERECALC
    arguments: cp, cch, tree-node;
    targets: self, ancestors, tree-level;
    categories: layout-elements;
    properties: send-until-handled, lazy-range, dont-block, clean-change;

type: ELEMENT_ENSURERECALC
    arguments: element, flags;
    targets: self, ancestors, tree-level;
    categories: layout-elements;
    properties: send-until-handled, lazy-range, dont-block, clean-change;

type: ELEMENT_INVALIDATE;
    arguments: element, flags;
    targets: self, ancestors;
    categories: layout-change, layout-elements, clean-change;
    properties: send-until-handled, lazy-range;

type: ELEMENT_INVAL_Z_DESCENDANTS
    arguments:element, flags
    targets: self, descendents, tree-level;
    categories: layout-change, clean-change, all-elements
    properties: lazy-range, zparents-only;

type: ELEMENT_MINMAX;
    arguments: element, flags;
    targets: self, ancestors;
    categories: layout-change, layout-elements;
    properties: send-until-handled, lazy-range, clean-change;

type: ELEMENT_ENTERTREE;
    arguments: element, tree-node, flags;
    targets: self;
    categories: tree-change;
    properties: lazy-range;

type: ELEMENT_EXITTREE;
    arguments: element, flags;
    targets: self;
    categories: tree-change;
    properties: lazy-range, second-chance;

type: ELEMENT_QUERYFOCUSSABLE
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_QUERYTABBABLE
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_QUERYMNEMONICTARGET
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_SETTINGFOCUS;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_SETFOCUS;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_SETFOCUSFAILED;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_GOTMNEMONIC;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: ELEMENT_LOSTMNEMONIC;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;

type: DISPLAY_CHANGE;
    arguments: element, flags;
    targets: self, descendents, tree-level;
    categories: layout-change;
    properties: lazy-range;

type: VISIBILITY_CHANGE;
    arguments: element, flags;
    targets: self, descendents;
    categories:  layout-change;
    properties: lazy-range;

type: ZPARENT_CHANGE;
    arguments: element, flags;
    targets: descendents;
    categories: positioned-elements;
    properties: lazy-range, zparents-only;

type: MEASURED_RANGE;
    arguments: cp, cch, flags;
    targets: tree-level;
    categories: positioned-elements;
    properties: auto-only, zparents-only;

type: TRANSLATED_RANGE;
    arguments: cp, cch, flags;
    targets: descendents, tree-level;
    categories: positioned-elements;
    properties: auto-only, zparents-only;

type: UPDATE_DOC_DIRTY;
    arguments: element, flags;
    targets: descendents;
    categories: activex;
    properties: lazy-range, clean-change;

type: FREEZE_EVENTS;
    arguments: element, flags, data;
    targets: descendents;
    categories: activex;
    properties: dont-block, lazy-range, clean-change ;

type: AMBIENT_PROP_CHANGE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: UPDATE_VIEW_CACHE;
    arguments: element, flags;
    targets: descendents;
    categories: activex;
    properties: lazy-range;

type: UPDATE_DOC_UPTODATE;
    arguments: element, flags;
    targets: descendents;
    categories: activex;
    properties: lazy-range;

type: DOC_STATE_CHANGE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range, clean-change, second-chance ;

type: SELECT_CHANGE;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;
    properties: lazy-range;

type: STOP;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range, second-chance;

type: ENABLE_INTERACTION;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range, second-chance;

type: ACTIVE_MOVIE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: BEFORE_UNLOAD;
    arguments: element, flags, data;
    targets: descendents;
    categories: activex;
    properties: lazy-range;

type: BEFORE_REFRESH;
    arguments: element, flags, data;
    targets: descendents;
    categories: activex;
    properties: lazy-range;

type: EDIT_MODE_CHANGE;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: SET_CODEPAGE;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: GET_FRAME_ZONE;
    arguments: element, flags, data;
    targets: descendents;
    categories: activex;
    properties: lazy-range;

type: DOC_END_PARSE;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range, clean-change;

type: RELEASE_EXTERNAL_OBJECTS;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: AREA_FOCUS;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: AREA_TABINDEX_CHANGE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: GET_START_ELEMENT;
    arguments: element, flags, data;
    targets: self;
    categories: all-elements;
    properties: lazy-range;

type: COMMAND;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: SNAP_SHOT_SAVE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: FAVORITES_SAVE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: FAVORITES_LOAD;
    arguments:element, flags, data;
    targets:descendents;
    categories: all-elements;
    properties: lazy-range;

type: XTAG_HISTORY_SAVE;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: XTAG_HISTORY_LOAD;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: KILL_SELECTION;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: SAVE_HISTORY;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range, synchronous-only, second-chance;

type: DELAY_LOAD_HISTORY;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: CLEAR_DIRTY;
    arguments: element, flags;
    targets: self;
    categories: all-elements;
    properties: lazy-range;

type: END_PARSE;
    arguments: element, flags;
    targets: self;
    categories: all-elements;
    properties: lazy-range ;

type: CLEAR_FORMAT_CACHES;
    arguments: element, flags, data;
    targets: descendents;
    categories: all-elements, layout-change, layout-elements;
    properties: lazy-range;

type: RECOMPUTE_BEHAVIOR;
    arguments: element, flags;
    targets: descendents;
    categories: all-elements;
    properties: lazy-range;

type: VIEW_ATTACHELEMENT
    arguments: element;
    targets: tree-level;
    categories: layout-change;
    properties: lazy-range;

type: VIEW_DETACHELEMENT
    arguments: element;
    targets: tree-level;
    categories: layout-change;
    properties: lazy-range;

type: BASE_URL_CHANGE
    arguments: element;
    targets: descendents;
    categories: all-elements;

type: ZERO_GRAY_CHANGE
    arguments: element;
    targets: descendents;
    categories: layout-elements;
    properties: lazy-range;
