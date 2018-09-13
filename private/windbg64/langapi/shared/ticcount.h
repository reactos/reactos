#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

class CTickCount {
public:
    inline      CTickCount(FILE * pfileOut);
    inline      ~CTickCount();
    inline void LapTime ( LPCTSTR szMsg = 0 );
    inline void FinalTime();
    inline bool FEnable(bool);
    inline void Reset();

private:
    DWORD       m_tickStart;
    DWORD       m_tickLap;
    FILE *      m_pfileOut;
    unsigned    m_cLaps;
    bool        m_fEnabled;

};

inline
CTickCount::CTickCount(FILE * pfileOut) {
	m_pfileOut = pfileOut;
	m_fEnabled = false;
    m_cLaps = 0;
	Reset();
}

inline
CTickCount::~CTickCount() {
    FinalTime();
}

inline
void
CTickCount::FinalTime() {
	DWORD	tickLapT = m_tickLap;
	m_tickLap = ::GetTickCount();
	
	DWORD	tickTotalT = m_tickLap - m_tickStart;
	tickLapT = m_tickLap - tickLapT;

	if ( m_fEnabled ) {
		_ftprintf (
			m_pfileOut,
            _TEXT("Final: Total time = %d.%03ds\n"),
            tickTotalT / 1000,
            tickTotalT % 1000
			);
        fflush ( m_pfileOut );
    }
    m_fEnabled = false;
}

inline
void
CTickCount::LapTime(LPCTSTR szMsg) {
	DWORD	tickLapT = m_tickLap;
	m_tickLap = ::GetTickCount();
	
	DWORD	tickTotalT = m_tickLap - m_tickStart;
	tickLapT = m_tickLap - tickLapT;

	if ( m_fEnabled ) {
		if ( szMsg ) {
			_ftprintf ( m_pfileOut, _TEXT("%s: "), szMsg );
		}
		_ftprintf (
			m_pfileOut,
            _TEXT("Interval #%d, time = %d.%03ds\n"),
            ++m_cLaps,
            tickLapT / 1000,
            tickLapT % 1000
			);
		fflush ( m_pfileOut );
	}
}

inline
bool
CTickCount::FEnable ( bool f ) {
	bool	fT = m_fEnabled;
	m_fEnabled = f;
	return fT;
}

inline
void
CTickCount::Reset() {
	m_tickLap = m_tickStart = ::GetTickCount();
}

#if defined(TEST_TICCOUNT)
int main() {
	CTickCount	tickMe(stdout);

	tickMe.FEnable(true);
	tickMe.LapTime("Pass 1");
	Sleep(100);
	tickMe.LapTime("Pass 2");
	Sleep(200);
	tickMe.LapTime("Pass 3");
	Sleep(300);
	tickMe.LapTime("Pass 4");
	Sleep(400);
	tickMe.LapTime("Pass 5");
	Sleep(500);
    tickMe.LapTime("Pass 6");
	return 0;
}
#endif
