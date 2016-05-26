/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_format.hpp
 * PURPOSE:         Audio format abstraction
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#ifndef _AUDIOFORMAT__H_
#define _AUDIOFORMAT__H_

#include "audio_def.hpp"

_AUDIO_NAMESPACE_START_

class audio_format
{
    protected:
        unsigned int samples_psec;
        unsigned short int bits_psample;
        unsigned short int chan;
    public:
        /* Ctors */
        audio_format(unsigned int samples_per_second,
                     unsigned short int bits_per_sample,
                     unsigned short int channels) : samples_psec(samples_per_second),
                                                    bits_psample(bits_per_sample),
                                                    chan(channels)
        {
        }

        /* Dtor */
        virtual ~audio_format(void)
        {
        }

        /* Operators */
        bool operator==(audio_format & eq) const
        {
            /* The same audio format is when samples per second,
               bit per sample, and channels mono/stereo are equal */
            return ((samples_psec == eq.samples_psec) &&
                    (bits_psample == eq.bits_psample) &&
                    (chan == eq.chan));
        }

        /* Public Functions */

        unsigned int sample_rate(void) const
        {
            return samples_psec;
        }

        unsigned short int bits(void) const
        {
            return bits_psample;
        }

        unsigned short int channels(void) const
        {
            return chan;
        }

        unsigned int byte_rate(void) const
        {
            return (samples_psec * chan * (bits_psample / 8));
        }

        unsigned int block_align(void) const
        {
            return (chan * (bits_psample / 8));
        }

        unsigned int samples_in_seconds(float seconds) const
        {
            return (unsigned int)(((float)samples_psec * (float) chan) * seconds);
        }

        unsigned int samples_in_bytes(unsigned int bytes) const
        {
            return (bytes / ((bits_psample / 8) * chan));
        }

        unsigned int bytes_in_samples(unsigned int samples) const
        {
            return (samples * ((bits_psample / 8) * chan));
        }
};

extern audio_format UNKNOWN_FORMAT;
extern audio_format A44100_16BIT_STEREO;
extern audio_format A44100_16BIT_MONO;

_AUDIO_NAMESPACE_END_

#endif /* _AUDIOFORMAT__H_ */
