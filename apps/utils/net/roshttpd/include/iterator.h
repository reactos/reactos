/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS HTTP Daemon
 * FILE:        include/iterator.h
 */
#ifndef __ITERATOR_H
#define __ITERATOR_H

#include <windows.h>

template <class Item>
class CIterator {
public:
	virtual VOID First() = 0;
	virtual VOID Next() = 0;
	virtual BOOL IsDone() const = 0;
	virtual Item CurrentItem() const = 0;
};

#endif /* __ITERATOR_H */
