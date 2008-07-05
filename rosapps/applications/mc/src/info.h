#ifndef __INFO_H
#define __INFO_H

typedef struct {
    Widget widget;
    int ready;
} WInfo;

WInfo *info_new ();

#endif
