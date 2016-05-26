/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_producer.hpp
 * PURPOSE:         Audio producer
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#ifndef _AUDIOAUDBUF__H_
#define _AUDIOAUDBUF__H_

#include "audio_def.hpp"

_AUDIO_NAMESPACE_START_

class audio_producer
{
    protected:
        unsigned int bytes_played_;

    public:
        /* Ctors */
        audio_producer() : bytes_played_(0), play_finished(0)
        {
        }

        /* Dtor */
        virtual ~audio_producer(void)
        {
        }

        /* Public Functions */

        /* reads N bytes from the buffer */
        virtual unsigned int read(BYTE *, unsigned int) = 0;

        virtual bool finished(void) = 0;

        unsigned int bytes_played(void) const
        {
            return bytes_played_;
        }

        void set_position(unsigned int pos)
        {
            bytes_played_ = pos;
        }

        void set_position_start(void)
        {
            bytes_played_ = 0;
        }

        void forward(unsigned int bytes)
        {
            bytes_played_ += bytes;
        }

        void backward(unsigned int bytes)
        {
            bytes_played_ += bytes;
        }

        void (* play_finished)(void);
};

_AUDIO_NAMESPACE_END_

#endif /* _AUDIOAUDBUF__H_ */
