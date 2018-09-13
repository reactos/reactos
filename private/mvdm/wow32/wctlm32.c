    case EM_GETSEL:             //                          0x00B0
    case EM_SETSEL:             //                          0x00B1
    case EM_GETRECT:             //                         0x00B2
    case EM_SETRECT:             //                         0x00B3
    case EM_SETRECTNP:             //                       0x00B4
    case EM_SCROLL:             //                          0x00B5
    case EM_LINESCROLL:             //                      0x00B6
    case EM_GETMODIFY:             //                       0x00B8
    case EM_SETMODIFY:             //                       0x00B9
    case EM_GETLINECOUNT:             //                    0x00BA
    case EM_LINEINDEX:             //                       0x00BB
    case EM_SETHANDLE:             //                       0x00BC
    case EM_GETHANDLE:             //                       0x00BD
    case EM_GETTHUMB:             //                        0x00BE
    case EM_LINELENGTH:             //                      0x00C1
    case EM_REPLACESEL:             //                      0x00C2
    case EM_SETFONT:             //                         0x00C3
    case EM_GETLINE:             //                         0x00C4
    case EM_LIMITTEXT:             //                       0x00C5
    case EM_CANUNDO:             //                         0x00C6
    case EM_UNDO:             //                            0x00C7
    case EM_FMTLINES:             //                        0x00C8
    case EM_LINEFROMCHAR:             //                    0x00C9
    case EM_SETWORDBREAK:             //                    0x00CA
    case EM_SETTABSTOPS:             //                     0x00CB
    case EM_SETPASSWORDCHAR:             //                 0x00CC
    case EM_EMPTYUNDOBUFFER:             //                 0x00CD
    case EM_GETFIRSTVISIBLE:             //                 0x00CE
    case EM_SETREADONLY:             //                     0x00CF
    case EM_MSGMAX:             //                          0x00D0
        return ThunkEMMsg32(hwnd, uMsg, uParam, lParam,
                                pwMsgNew, pwParamNew, plParamNew);

    case SBM_SETPOS:             //                         0x00E0
    case SBM_GETPOS:             //                         0x00E1
    case SBM_SETRANGE:             //                       0x00E2
    case SBM_GETRANGE:             //                       0x00E3
    case SBM_ENABLE_ARROWS:             //                  0x00E4
        return ThunkSBMMsg32(hwnd, uMsg, uParam, lParam,
                                 pwMsgNew, pwParamNew, plParamNew);


    case BM_GETCHECK:             //                        0x00F0
    case BM_SETCHECK:             //                        0x00F1
    case BM_GETSTATE:             //                        0x00F2
    case BM_SETSTATE:             //                        0x00F3
    case BM_SETSTYLE:             //                        0x00F4
        return ThunkBMMsg32(hwnd, uMsg, uParam, lParam,
                                 pwMsgNew, pwParamNew, plParamNew);



    case CB_GETEDITSEL:             //                      0x0140
    case CB_LIMITTEXT:             //                       0x0141
    case CB_SETEDITSEL:             //                      0x0142
    case CB_ADDSTRING:             //                       0x0143
    case CB_DELETESTRING:             //                    0x0144
    case CB_DIR:             //                             0x0145
    case CB_GETCOUNT:             //                        0x0146
    case CB_GETCURSEL:             //                       0x0147
    case CB_GETLBTEXT:             //                       0x0148
    case CB_GETLBTEXTLEN:             //                    0x0149
    case CB_INSERTSTRING:             //                    0x014A
    case CB_RESETCONTENT:             //                    0x014B
    case CB_FINDSTRING:             //                      0x014C
    case CB_SELECTSTRING:             //                    0x014D
    case CB_SETCURSEL:             //                       0x014E
    case CB_SHOWDROPDOWN:             //                    0x014F
    case CB_GETITEMDATA:             //                     0x0150
    case CB_SETITEMDATA:             //                     0x0151
    case CB_GETDROPPEDCONTROLRECT:             //           0x0152
    case CB_SETITEMHEIGHT:             //                   0x0153
    case CB_GETITEMHEIGHT:             //                   0x0154
    case CB_SETEXTENDEDUI:             //                   0x0155
    case CB_GETEXTENDEDUI:             //                   0x0156
    case CB_GETDROPPEDSTATE:             //                 0x0157
    case CB_MSGMAX:             //                          0x0158
        return ThunkCBMsg32(hwnd, uMsg, uParam, lParam,
                                 pwMsgNew, pwParamNew, plParamNew);



    case LB_ADDSTRING:             //                       0x0180
    case LB_INSERTSTRING:             //                    0x0181
    case LB_DELETESTRING:             //                    0x0182
    case LB_RESETCONTENT:             //                    0x0184
    case LB_SETSEL:             //                          0x0185
    case LB_SETCURSEL:             //                       0x0186
    case LB_GETSEL:             //                          0x0187
    case LB_GETCURSEL:             //                       0x0188
    case LB_GETTEXT:             //                         0x0189
    case LB_GETTEXTLEN:             //                      0x018A
    case LB_GETCOUNT:             //                        0x018B
    case LB_SELECTSTRING:             //                    0x018C
    case LB_DIR:             //                             0x018D
    case LB_GETTOPINDEX:             //                     0x018E
    case LB_FINDSTRING:             //                      0x018F
    case LB_GETSELCOUNT:             //                     0x0190
    case LB_GETSELITEMS:             //                     0x0191
    case LB_SETTABSTOPS:             //                     0x0192
    case LB_GETHORIZONTALEXTENT:             //             0x0193
    case LB_SETHORIZONTALEXTENT:             //             0x0194
    case LB_SETCOLUMNWIDTH:             //                  0x0195
    case LB_SETTOPINDEX:             //                     0x0197
    case LB_GETITEMRECT:             //                     0x0198
    case LB_GETITEMDATA:             //                     0x0199
    case LB_SETITEMDATA:             //                     0x019A
    case LB_SELITEMRANGE:             //                    0x019B
    case LB_SETITEMHEIGHT:             //                   0x01A0
    case LB_GETITEMHEIGHT:             //                   0x01A1
    case LBCB_CARETON:                 //                   0x01A3
    case LBCB_CARETOFF:                //                   0x01A4
    case LB_MSGMAX:             //                          0x01A5
        return ThunkLBMsg32(hwnd, uMsg, uParam, lParam,
                                 pwMsgNew, pwParamNew, plParamNew);


BOOL ThunkEMMsg32(HWND hwnd, UINT uMsg, UINT uParam, LONG lParam,
                  PWORD pwMsgNew, PWORD pwParamNew, PLONG plParamNew)
{
    // case EM_GETSEL:             //                          0x00B0
    // case EM_SETSEL:             //                          0x00B1
    // case EM_GETRECT:             //                         0x00B2
    // case EM_SETRECT:             //                         0x00B3
    // case EM_SETRECTNP:             //                       0x00B4
    // case EM_SCROLL:             //                          0x00B5
    // case EM_LINESCROLL:             //                      0x00B6
    // case EM_GETMODIFY:             //                       0x00B8
    // case EM_SETMODIFY:             //                       0x00B9
    // case EM_GETLINECOUNT:             //                    0x00BA
    // case EM_LINEINDEX:             //                       0x00BB
    // case EM_SETHANDLE:             //                       0x00BC
    // case EM_GETHANDLE:             //                       0x00BD
    // case EM_GETTHUMB:             //                        0x00BE
    // case EM_LINELENGTH:             //                      0x00C1
    // case EM_REPLACESEL:             //                      0x00C2
    // case EM_SETFONT:             //                         0x00C3
    // case EM_GETLINE:             //                         0x00C4
    // case EM_LIMITTEXT:             //                       0x00C5
    // case EM_CANUNDO:             //                         0x00C6
    // case EM_UNDO:             //                            0x00C7
    // case EM_FMTLINES:             //                        0x00C8
    // case EM_LINEFROMCHAR:             //                    0x00C9
    // case EM_SETWORDBREAK:             //                    0x00CA
    // case EM_SETTABSTOPS:             //                     0x00CB
    // case EM_SETPASSWORDCHAR:             //                 0x00CC
    // case EM_EMPTYUNDOBUFFER:             //                 0x00CD
    // case EM_GETFIRSTVISIBLE:             //                 0x00CE
    // case EM_SETREADONLY:             //                     0x00CF
    // case EM_MSGMAX:             //                          0x00D0


    *pwMsgNew = WM_USER + (uMsg - EM_GETSEL); // EM_GETSEL is the base

    switch(uMsg) {
        case EM_GETSEL:             //                          0x00B0
            *pwParamNew = (WORD)0;
            *plParamNew = (LONG)0;
            break;

        case EM_SETSEL:             //                          0x00B1
            LOW(*plParamNew) = (WORD)((SHORT)uParam);
            HIW(*plParamNew) = (WORD)((SHORT)lParam);
            break;

        case EM_GETRECT:             //                         0x00B2
            *plParamNew = GlobalAllocLock16(GMEM_MOVEABLE,
                                                     sizeof(RECT16), NULL);
            if (!(*plParamNew))
                return FALSE;

            break;

        case EM_SETRECT:             //                         0x00B3
        case EM_SETRECTNP:             //                       0x00B4
            if (lParam) {
                *plParamNew = GlobalAllocLock16(GMEM_MOVEABLE,
                                                         sizeof(RECT16), NULL);
                if (!(*plParamNew))
                    return FALSE;
                putrect16((VPRECT16)*plParamNew, (LPRECT)lParam);
            }
            break;

        case EM_LINESCROLL:             //                      0x00B6
            LOW(*plParamNew) = (WORD)(uParam);
            HIW(*plParamNew) = (WORD)(lParam);
            break;

        case EM_SETHANDLE:             //                       0x00BC
        case EM_GETHANDLE:             //                       0x00BD
            LOGDEBUG(0, "ThunkEMMsg32:EM_xxxHANDLE - What to do\n");
            break;

        case EM_REPLACESEL:             //                      0x00C2
            if (lParam) {
                INT cb;

                cb = strlen((LPSZ)lParam+1);
                *plParamNew = GlobalAllocLock16(GMEM_MOVEABLE, cb, NULL);
                if (!(*plParamNew))
                    return FALSE;
                putstr16((VPSZ16)*plParamNew, (LPSZ)lParam, cb);
            }
            break;

        case EM_SETFONT:             //                         0x00C3
            LOGDEBUG(0, "ThunkEMMsg32:EM_SETFONT - What to do\n");
            break;

        case EM_GETLINE:             //                         0x00C4
*************************************

        case EM_SETWORDBREAK:             //                    0x00CA
            LOGDEBUG(0, "ThunkEMMsg32:EM_SETWORDBREAK - What to do\n");
            break;

        case EM_SETTABSTOPS:             //                     0x00CB
            if (wParam != 0) {
                *plParamNew = GlobalAllocLock16(GMEM_MOVEABLE,
                                                  wParam * sizeof(WORD), NULL);
                if (!(*plParamNew))
                    return FALSE;
**********************putrect16((VPRECT16)*plParamNew, (LPRECT)lParam);
            }
            break;

    }

    return TRUE;
}

