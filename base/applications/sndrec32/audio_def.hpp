/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_def.hpp
 * PURPOSE:         Winmm abstraction settings
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#ifndef _AUDIO_DEF__H_
#define _AUDIO_DEF__H_

/* Defaults */

#define _AUDIO_DEFAULT_FORMAT A44100_16BIT_STEREO

#define _AUDIO_DEFAULT_WAVEINBUFFERS 3
#define _AUDIO_DEFAULT_WAVEINBUFSECS 0.5f

#define _AUDIO_DEFAULT_WAVEOUTBUFFERS 3
#define _AUDIO_DEFAULT_WAVEOUTBUFSECS 0.1f

#define _AUDIO_DEFAULT_BUFSECS 1.0f

/* Namespace stuff */
#define _AUDIO_NAMESPACE_START_ namespace snd {
#define _AUDIO_NAMESPACE_END_ };

/* Platform depend stuff */
#include <mmsystem.h> // Windows MultiMedia (WINMM) audio apis
#include <mmreg.h> // codecs stuff
#include <msacm.h> // codecs stuff

//#pragma comment(lib, "winmm.lib")
//#pragma comment(lib, "msacm32.lib")

#endif //ifdef _AUDIO_DEF__H_
