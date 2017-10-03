/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_format.cpp
 * PURPOSE:         Sound recording
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#include "stdafx.h"
#include "audio_format.hpp"

_AUDIO_NAMESPACE_START_

/* Standard audio formats (declared as externs in `audio_format.hpp') */

audio_format UNNKOWN_FORMAT(0, 0, 0);
audio_format A44100_16BIT_STEREO(44100, 16, 2);
audio_format A44100_16BIT_MONO(44100, 16, 1);

_AUDIO_NAMESPACE_END_
