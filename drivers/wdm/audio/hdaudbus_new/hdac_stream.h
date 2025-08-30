UINT16 hdac_format(PHDAC_STREAM stream);

/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/hdaudbus/hdac_stream.h
 * PURPOSE:         HDAUDBUS Driver
 * PROGRAMMER:      Coolstar TODO
                    Johannes Anderwald
 */

void hdac_stream_start(PHDAC_STREAM stream);
void hdac_stream_stop(PHDAC_STREAM stream);
void hdac_stream_reset(PHDAC_STREAM stream);
void hdac_stream_setup(PHDAC_STREAM stream);
