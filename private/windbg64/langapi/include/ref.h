#pragma once

#if !defined(_ref_h)
#define _ref_h 1

#define refNil	0

template <class TARG>
class RefPtr {
  private:
	TARG *	_ptarg;

	// unuse pointer and free if last user
	void Release() {
		if (_ptarg != NULL && _ptarg->FUnUse()) {
			delete _ptarg;
			}

		// put garbage in pointer
		Debug(_ptarg = (TARG *) 0xdadadada);
		}

  public:
	// constructors
	RefPtr<TARG>() {
		_ptarg = NULL;
		}

	RefPtr<TARG>(TARG * ptargNew) {
		_ptarg = ptargNew;
		if (ptargNew != NULL) {
			ptargNew->Use();
			}
		}

	RefPtr<TARG>(const RefPtr<TARG> & rtarg) {
		_ptarg = rtarg._ptarg;
		if (rtarg._ptarg != NULL) {
			rtarg._ptarg->Use();
			}
		}

	// destructor
   ~RefPtr<TARG>() {
		Release();
		}

	// assignment functions (mirror the ctors)
	RefPtr<TARG>& operator=(TARG *ptargNew) {
		if (ptargNew != NULL) {
			ptargNew->Use();
			}

		Release();
		_ptarg = ptargNew;

		return *this;
		}

	RefPtr<TARG>& operator=(const RefPtr<TARG> &rtarg) {
		if (rtarg._ptarg != NULL) {
			rtarg._ptarg->Use();
			}

		Release();
		_ptarg = rtarg._ptarg;

		return *this;
		}

	bool  operator==(TARG *ptarg) const { return  _ptarg == ptarg; }
	bool  operator!=(TARG *ptarg) const { return  _ptarg != ptarg; }
	TARG &operator*()			  const { return *_ptarg;		   }
	TARG *operator->()			  const { return  _ptarg;		   }
		  operator TARG *() 	  const { return  _ptarg;		   }
	};

#endif
