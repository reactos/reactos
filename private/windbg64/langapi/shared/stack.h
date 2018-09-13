#ifndef __STACK_INCLUDED__
#define __STACK_INCLUDED__

#ifndef __SHARED_INCLUDED__
#include <shared.h>
#endif

#ifndef __ARRAY_INCLUDED__
#include <array_t.h>
#endif

template <class T> class Stack {
public:
	unsigned depth() const {
		return rgt.size();
	}
	BOOL isEmpty() const {
		return depth() == 0;
	}
	T& top() const {
		return rgt[depth() - 1];
	}
	BOOL push(T& t) {
		return rgt.append(t);
	}
	BOOL pop() {
		if (!isEmpty()) {
			top() = T();
			rgt.setSize(depth() - 1);
			return TRUE;
		} else {
			return FALSE;
		}
	}
private:		
	Array<T> rgt;
};

#endif // !__STACK_INCLUDED__
