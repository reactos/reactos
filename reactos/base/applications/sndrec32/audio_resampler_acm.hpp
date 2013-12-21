/* PROJECT:         ReactOS sndrec32
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/sndrec32/audio_resampler_acm.hpp
 * PURPOSE:         Windows ACM wrapper
 * PROGRAMMERS:     Marco Pagliaricci (irc: rendar)
 */

#ifndef _AUDIORESAMPLERACM__H_
#define _AUDIORESAMPLERACM__H_

#include "audio_receiver.hpp"
#include "audio_format.hpp"

_AUDIO_NAMESPACE_START_

/* TODO: inherit from a base resampler? */
class audio_resampler_acm : public audio_receiver
{
    private:
        void init_(void);

    protected:
        HACMSTREAM acm_stream;
        ACMSTREAMHEADER acm_header;
        DWORD src_buflen;
        DWORD dst_buflen;
        bool stream_opened;

        audio_format audfmt_in;
        audio_format audfmt_out;

        float buf_secs;

        WAVEFORMATEX wformat_src;
        WAVEFORMATEX wformat_dst;

    public:
        /* Ctors */
        audio_resampler_acm(audio_format fmt_in,
                            audio_format fmt_out) : acm_stream(0),
                                                    src_buflen(0),
                                                    dst_buflen(0),
                                                    stream_opened(false),
                                                    audfmt_in(fmt_in),
                                                    audfmt_out(fmt_out),
                                                    buf_secs(_AUDIO_DEFAULT_BUFSECS)
        {
            init_();
        }

        /* Dtor */
        ~audio_resampler_acm(void)
        {
        }

        /* Public functions */
        void open(void);
        void close(void);
        void audio_receive(unsigned char *, unsigned int);
};

_AUDIO_NAMESPACE_END_

#endif /* _AUDIORESAMPLERACM__H_ */
