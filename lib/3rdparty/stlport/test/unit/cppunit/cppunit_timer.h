/*
 * Copyright (c) 2006
 * Francois Dumont
 *
 * This material is provided "as is", with absolutely no warranty expressed
 * or implied. Any use is at your own risk.
 *
 * Permission to use or copy this software for any purpose is hereby granted
 * without fee, provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 *
 */

#ifndef CPPUNIT_TIMER_H
#define CPPUNIT_TIMER_H

#if defined (_WIN32)
#  define CPPUNIT_WIN32_TIMER
#  include <windows.h>
#endif

class Timer {
public:
  Timer() {
#if defined (CPPUNIT_WIN32_TIMER)
    m_start.LowPart = m_restart.LowPart = m_stop.LowPart = 0;
    m_start.HighPart = m_restart.HighPart = m_stop.HighPart = 0;
    QueryPerformanceFrequency(&m_frequency);
#endif
  }

  void start() {
#if defined (CPPUNIT_WIN32_TIMER)
    QueryPerformanceCounter(&m_start);
#endif
  }

  void restart() {
#if defined (CPPUNIT_WIN32_TIMER)
    QueryPerformanceCounter(&m_restart);
    if (m_start.HighPart == 0 && m_start.LowPart == 0) {
      m_start = m_restart;
    }
#endif
  }

  void stop() {
#if defined (CPPUNIT_WIN32_TIMER)
    LARGE_INTEGER stop;
    QueryPerformanceCounter(&stop);
    if ((m_stop.HighPart != 0 || m_stop.LowPart != 0) &&
        m_restart.HighPart != 0 && m_restart.LowPart != 0) {
      m_stop.HighPart += (stop.HighPart - m_restart.HighPart);
      if (stop.LowPart < m_restart.LowPart) {
        if (m_restart.LowPart - stop.LowPart > m_stop.LowPart) {
          m_stop.HighPart -= 1;
        }
        m_stop.LowPart -= m_restart.LowPart - stop.LowPart;
      }
      else {
        if (stop.LowPart - m_restart.LowPart > 0xFFFFFFFF - m_stop.LowPart) {
          m_stop.HighPart += 1;
        }
        m_stop.LowPart += stop.LowPart - m_restart.LowPart;
      }
    }
    else {
      m_stop = stop;
    }
#endif
  }

  double elapsedMilliseconds() const {
#if defined (CPPUNIT_WIN32_TIMER)
    LARGE_INTEGER elapsed;
    elapsed.HighPart = m_stop.HighPart - m_start.HighPart;
    elapsed.LowPart = m_stop.LowPart - m_start.LowPart;
    return (double)elapsed.QuadPart / (double)m_frequency.QuadPart * 1000;
#else
    return 0;
#endif
  }

  static bool supported() {
#if defined (CPPUNIT_WIN32_TIMER)
    return true;
#else
    return false;
#endif
  }

private:
#if defined (CPPUNIT_WIN32_TIMER)
  LARGE_INTEGER m_frequency;
  LARGE_INTEGER m_start, m_stop, m_restart;
#endif
};

#endif
