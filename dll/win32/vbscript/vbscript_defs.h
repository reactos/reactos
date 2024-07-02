/*
 * Copyright 2011 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define DISPID_GLOBAL_VBUSESYSTEM          0
#define DISPID_GLOBAL_USESYSTEMDAYOFWEEK   1
#define DISPID_GLOBAL_VBSUNDAY             2
#define DISPID_GLOBAL_VBMONDAY             3
#define DISPID_GLOBAL_VBTUESDAY            4
#define DISPID_GLOBAL_VBWEDNESDAY          5
#define DISPID_GLOBAL_VBTHURSDAY           6
#define DISPID_GLOBAL_VBFRIDAY             7
#define DISPID_GLOBAL_VBSATURDAY           8
#define DISPID_GLOBAL_VBFIRSTJAN1          9
#define DISPID_GLOBAL_VBFIRSTFOURDAYS      10
#define DISPID_GLOBAL_VBFIRSTFULLWEEK      11
#define DISPID_GLOBAL_VBOKONLY             12
#define DISPID_GLOBAL_VBOKCANCEL           13
#define DISPID_GLOBAL_VBABORTRETRYIGNORE   14
#define DISPID_GLOBAL_VBYESNOCANCEL        15
#define DISPID_GLOBAL_VBYESNO              16
#define DISPID_GLOBAL_VBRETRYCANCEL        17
#define DISPID_GLOBAL_VBCRITICAL           18
#define DISPID_GLOBAL_VBQUESTION           19
#define DISPID_GLOBAL_VBEXCLAMATION        20
#define DISPID_GLOBAL_VBINFORMATION        21
#define DISPID_GLOBAL_VBDEFAULTBUTTON1     22
#define DISPID_GLOBAL_VBDEFAULTBUTTON2     23
#define DISPID_GLOBAL_VBDEFAULTBUTTON3     24
#define DISPID_GLOBAL_VBDEFAULTBUTTON4     25
#define DISPID_GLOBAL_VBAPPLICATIONMODAL   26
#define DISPID_GLOBAL_VBSYSTEMMODAL        27
#define DISPID_GLOBAL_VBOK                 28
#define DISPID_GLOBAL_VBCANCEL             29
#define DISPID_GLOBAL_VBABORT              30
#define DISPID_GLOBAL_VBRETRY              31
#define DISPID_GLOBAL_VBIGNORE             32
#define DISPID_GLOBAL_VBYES                33
#define DISPID_GLOBAL_VBNO                 34
#define DISPID_GLOBAL_VBEMPTY              35
#define DISPID_GLOBAL_VBNULL               36
#define DISPID_GLOBAL_VBINTEGER            37
#define DISPID_GLOBAL_VBLONG               38
#define DISPID_GLOBAL_VBSINGLE             39
#define DISPID_GLOBAL_VBDOUBLE             40
#define DISPID_GLOBAL_VBCURRENCY           41
#define DISPID_GLOBAL_VBDATE               42
#define DISPID_GLOBAL_VBSTRING             43
#define DISPID_GLOBAL_VBOBJECT             44
#define DISPID_GLOBAL_VBERROR              45
#define DISPID_GLOBAL_VBBOOLEAN            46
#define DISPID_GLOBAL_VBVARIANT            47
#define DISPID_GLOBAL_VBDATAOBJECT         48
#define DISPID_GLOBAL_VBDECIMAL            49
#define DISPID_GLOBAL_VBBYTE               50
#define DISPID_GLOBAL_VBARRAY              51
#define DISPID_GLOBAL_VBTRUE               52
#define DISPID_GLOBAL_VBFALSE              53
#define DISPID_GLOBAL_VBUSEDEFAULT         54
#define DISPID_GLOBAL_VBBINARYCOMPARE      55
#define DISPID_GLOBAL_VBTEXTCOMPARE        56
#define DISPID_GLOBAL_VBDATABASECOMPARE    57
#define DISPID_GLOBAL_VBGENERALDATE        58
#define DISPID_GLOBAL_VBLONGDATE           59
#define DISPID_GLOBAL_VBSHORTDATE          60
#define DISPID_GLOBAL_VBLONGTIME           61
#define DISPID_GLOBAL_VBSHORTTIME          62
#define DISPID_GLOBAL_VBOBJECTERROR        63
#define DISPID_GLOBAL_VBBLACK              64
#define DISPID_GLOBAL_VBBLUE               65
#define DISPID_GLOBAL_VBCYAN               66
#define DISPID_GLOBAL_VBGREEN              67
#define DISPID_GLOBAL_VBMAGENTA            68
#define DISPID_GLOBAL_VBRED                69
#define DISPID_GLOBAL_VBWHITE              70
#define DISPID_GLOBAL_VBYELLOW             71
#define DISPID_GLOBAL_VBCR                 72
#define DISPID_GLOBAL_VBCRLF               73
#define DISPID_GLOBAL_VBNEWLINE            74
#define DISPID_GLOBAL_VBFORMFEED           75
#define DISPID_GLOBAL_VBLF                 76
#define DISPID_GLOBAL_VBNULLCHAR           77
#define DISPID_GLOBAL_VBNULLSTRING         78
#define DISPID_GLOBAL_VBTAB                79
#define DISPID_GLOBAL_VBVERTICALTAB        80

#define DISPID_GLOBAL_VBMSGBOXHELPBUTTON        207
#define DISPID_GLOBAL_VBMSGBOXSETFOREGROUND     208
#define DISPID_GLOBAL_VBMSGBOXRIGHT             209
#define DISPID_GLOBAL_VBMSGBOXRTLREADING        210

#define DISPID_GLOBAL_CCUR                      100
#define DISPID_GLOBAL_CINT                      101
#define DISPID_GLOBAL_CLNG                      102
#define DISPID_GLOBAL_CBOOL                     103
#define DISPID_GLOBAL_CBYTE                     104
#define DISPID_GLOBAL_CDATE                     105
#define DISPID_GLOBAL_CDBL                      106
#define DISPID_GLOBAL_CSNG                      107
#define DISPID_GLOBAL_CSTR                      108
#define DISPID_GLOBAL_HEX                       109
#define DISPID_GLOBAL_OCT                       110
#define DISPID_GLOBAL_VARTYPE                   111
#define DISPID_GLOBAL_ISDATE                    112
#define DISPID_GLOBAL_ISEMPTY                   113
#define DISPID_GLOBAL_ISNULL                    114
#define DISPID_GLOBAL_ISNUMERIC                 115
#define DISPID_GLOBAL_ISARRAY                   116
#define DISPID_GLOBAL_ISOBJECT                  117
#define DISPID_GLOBAL_ATN                       118
#define DISPID_GLOBAL_COS                       119
#define DISPID_GLOBAL_SIN                       120
#define DISPID_GLOBAL_TAN                       121
#define DISPID_GLOBAL_EXP                       122
#define DISPID_GLOBAL_LOG                       123
#define DISPID_GLOBAL_SQR                       124
#define DISPID_GLOBAL_RANDOMIZE                 125
#define DISPID_GLOBAL_RND                       126
#define DISPID_GLOBAL_TIMER                     127
#define DISPID_GLOBAL_LBOUND                    128
#define DISPID_GLOBAL_UBOUND                    129
#define DISPID_GLOBAL_RGB                       130
#define DISPID_GLOBAL_LEN                       131
#define DISPID_GLOBAL_LENB                      132
#define DISPID_GLOBAL_LEFT                      133
#define DISPID_GLOBAL_LEFTB                     134
#define DISPID_GLOBAL_RIGHT                     135
#define DISPID_GLOBAL_RIGHTB                    136
#define DISPID_GLOBAL_MID                       137
#define DISPID_GLOBAL_MIDB                      138
#define DISPID_GLOBAL_STRCOMP                   139
#define DISPID_GLOBAL_LCASE                     140
#define DISPID_GLOBAL_UCASE                     141
#define DISPID_GLOBAL_LTRIM                     142
#define DISPID_GLOBAL_RTRIM                     143
#define DISPID_GLOBAL_TRIM                      144
#define DISPID_GLOBAL_SPACE                     145
#define DISPID_GLOBAL_STRING                    146
#define DISPID_GLOBAL_INSTR                     147
#define DISPID_GLOBAL_INSTRB                    148
#define DISPID_GLOBAL_ASCB                      149
#define DISPID_GLOBAL_CHRB                      150
#define DISPID_GLOBAL_ASC                       151
#define DISPID_GLOBAL_CHR                       152
#define DISPID_GLOBAL_ASCW                      153
#define DISPID_GLOBAL_CHRW                      154
#define DISPID_GLOBAL_ABS                       155
#define DISPID_GLOBAL_FIX                       156
#define DISPID_GLOBAL_INT                       157
#define DISPID_GLOBAL_SGN                       158
#define DISPID_GLOBAL_NOW                       159
#define DISPID_GLOBAL_DATE                      160
#define DISPID_GLOBAL_TIME                      161
#define DISPID_GLOBAL_DAY                       162
#define DISPID_GLOBAL_MONTH                     163
#define DISPID_GLOBAL_WEEKDAY                   164
#define DISPID_GLOBAL_YEAR                      165
#define DISPID_GLOBAL_HOUR                      166
#define DISPID_GLOBAL_MINUTE                    167
#define DISPID_GLOBAL_SECOND                    168
#define DISPID_GLOBAL_DATEVALUE                 169
#define DISPID_GLOBAL_TIMEVALUE                 170
#define DISPID_GLOBAL_DATESERIAL                171
#define DISPID_GLOBAL_TIMESERIAL                172
#define DISPID_GLOBAL_INPUTBOX                  173
#define DISPID_GLOBAL_MSGBOX                    174
#define DISPID_GLOBAL_CREATEOBJECT              175
#define DISPID_GLOBAL_GETOBJECT                 176
#define DISPID_GLOBAL_DATEADD                   177
#define DISPID_GLOBAL_DATEDIFF                  178
#define DISPID_GLOBAL_DATEPART                  179
#define DISPID_GLOBAL_TYPENAME                  180
#define DISPID_GLOBAL_ARRAY                     181
#define DISPID_GLOBAL_ERASE                     182
#define DISPID_GLOBAL_FILTER                    183
#define DISPID_GLOBAL_JOIN                      184
#define DISPID_GLOBAL_SPLIT                     185
#define DISPID_GLOBAL_REPLACE                   186
#define DISPID_GLOBAL_STRREVERSE                187
#define DISPID_GLOBAL_INSTRREV                  188
#define DISPID_GLOBAL_LOADPICTURE               189
#define DISPID_GLOBAL_SCRIPTENGINE              190
#define DISPID_GLOBAL_SCRIPTENGINEMAJORVERSION  191
#define DISPID_GLOBAL_SCRIPTENGINEMINORVERSION  192
#define DISPID_GLOBAL_SCRIPTENGINEBUILDVERSION  193
#define DISPID_GLOBAL_FORMATNUMBER              194
#define DISPID_GLOBAL_FORMATCURRENCY            195
#define DISPID_GLOBAL_FORMATPERCENT             196
#define DISPID_GLOBAL_FORMATDATETIME            197
#define DISPID_GLOBAL_WEEKDAYNAME               198
#define DISPID_GLOBAL_MONTHNAME                 199
#define DISPID_GLOBAL_ROUND                     200
#define DISPID_GLOBAL_ESCAPE                    201
#define DISPID_GLOBAL_UNESCAPE                  202
#define DISPID_GLOBAL_EVAL                      203
#define DISPID_GLOBAL_EXECUTE                   204
#define DISPID_GLOBAL_EXECUTEGLOBAL             205
#define DISPID_GLOBAL_GETREF                    206

#define DISPID_ERR_DESCRIPTION  0
#define DISPID_ERR_HELPCONTEXT  1
#define DISPID_ERR_HELPFILE     2
#define DISPID_ERR_NUMBER       3
#define DISPID_ERR_SOURCE       4
#define DISPID_ERR_CLEAR        100
#define DISPID_ERR_RAISE        101

#define DISPID_SUBMATCHES_COUNT     1

#define DISPID_MATCHCOLLECTION_COUNT    1

#define DISPID_MATCH_FIRSTINDEX     10001
#define DISPID_MATCH_LENGTH         10002
#define DISPID_MATCH_SUBMATCHES     10003

#define DISPID_REGEXP_PATTERN       10001
#define DISPID_REGEXP_IGNORECASE    10002
#define DISPID_REGEXP_GLOBAL        10003
#define DISPID_REGEXP_EXECUTE       10004
#define DISPID_REGEXP_TEST          10005
#define DISPID_REGEXP_REPLACE       10006
#define DISPID_REGEXP_MULTILINE     10007

/* error codes */
#define VBSE_ILLEGAL_FUNC_CALL              5
#define VBSE_OVERFLOW                       6
#define VBSE_OUT_OF_MEMORY                  7
#define VBSE_OUT_OF_BOUNDS                  9
#define VBSE_ARRAY_LOCKED                  10
#define VBSE_TYPE_MISMATCH                 13
#define VBSE_FILE_NOT_FOUND                53
#define VBSE_IO_ERROR                      57
#define VBSE_FILE_ALREADY_EXISTS           58
#define VBSE_DISK_FULL                     61
#define VBSE_TOO_MANY_FILES                67
#define VBSE_PERMISSION_DENIED             70
#define VBSE_PATH_FILE_ACCESS              75
#define VBSE_PATH_NOT_FOUND                76
#define VBSE_OBJECT_VARIABLE_NOT_SET       91
#define VBSE_ILLEGAL_NULL_USE              94
#define VBSE_CANT_CREATE_TMP_FILE         322
#define VBSE_CANT_CREATE_OBJECT           429
#define VBSE_OLE_NOT_SUPPORTED            430
#define VBSE_OLE_FILE_NOT_FOUND           432
#define VBSE_OLE_NO_PROP_OR_METHOD        438
#define VBSE_ACTION_NOT_SUPPORTED         445
#define VBSE_NAMED_ARGS_NOT_SUPPORTED     446
#define VBSE_LOCALE_SETTING_NOT_SUPPORTED 447
#define VBSE_NAMED_PARAM_NOT_FOUND        448
#define VBSE_PARAMETER_NOT_OPTIONAL       449
#define VBSE_FUNC_ARITY_MISMATCH          450
#define VBSE_NOT_ENUM                     451
#define VBSE_INVALID_DLL_FUNCTION_NAME    453
#define VBSE_INVALID_TYPELIB_VARIABLE     458
#define VBSE_SERVER_NOT_FOUND             462
#define VBSE_UNQUALIFIED_REFERENCE        505

#define VBS_COMPILE_ERROR                4096
#define VBS_RUNTIME_ERROR                4097
#define VBS_UNKNOWN_RUNTIME_ERROR        4098
