#include "driver.h"

void hdac_stream_start(PHDAC_STREAM stream) {
	hda_read32(stream->FdoContext, WALLCLK);

	/* enable SIE */
	hda_update32(stream->FdoContext, INTCTL, 1 << stream->idx, 1 << stream->idx);

	/* set stripe control */
	if (stream->stripe) {
		UINT8 stripe_ctl = 1;
		stream_update8(stream, SD_CTL_3B, SD_CTL_STRIPE_MASK,
			stripe_ctl);
	}

	/* set DMA start and interrupt mask */
	stream_update8(stream, SD_CTL, 0, SD_CTL_DMA_START | SD_INT_MASK);
	stream->running = TRUE;
}

void hdac_stream_clear(PHDAC_STREAM stream) {
	stream_update8(stream, SD_CTL, SD_CTL_DMA_START | SD_INT_MASK, 0);
	stream_write8(stream, SD_STS, SD_INT_MASK); /* to be sure */
	if (stream->stripe)
		stream_update8(stream, SD_CTL_3B, SD_CTL_STRIPE_MASK, 0);

	stream->running = FALSE;
}

void hdac_stream_stop(PHDAC_STREAM stream) {
	hdac_stream_clear(stream);

	/* disable SIE */
	hda_update32(stream->FdoContext, INTCTL, 1 << stream->idx, 0);
}

void hdac_stream_reset(PHDAC_STREAM stream) {
	hdac_stream_clear(stream);

	UINT8 dma_run_state;
	dma_run_state = stream_read8(stream, SD_CTL) & SD_CTL_DMA_START;

	stream_update8(stream, SD_CTL, 0, SD_CTL_STREAM_RESET);
	udelay(3);
	int timeout = 300;

	UCHAR val;
	do {
		val = stream_read8(stream, SD_CTL) & SD_CTL_STREAM_RESET;
		if (val)
			break;
	} while (--timeout);

	val &= ~SD_CTL_STREAM_RESET;
	stream_write8(stream, SD_CTL, val);
	udelay(3);

	timeout = 300;
	/* waiting for hardware to report that the stream is out of reset */
	do {
		val = stream_read8(stream, SD_CTL) & SD_CTL_STREAM_RESET;
		if (!val)
			break;
	} while (--timeout);

	if (stream->posbuf)
		*stream->posbuf = 0;
}

UINT16 hdac_format(PHDAC_STREAM stream) {
	UINT16 format = 0;

	switch (stream->streamFormat.SampleRate) {
	case 8000:
		format = HDA_RATE(48, 1, 6);
		break;
	case 9600:
		format = HDA_RATE(48, 1, 5);
		break;
	case 11025:
		format = HDA_RATE(44, 1, 4);
		break;
	case 16000:
		format = HDA_RATE(48, 1, 3);
		break;
	case 22050:
		format = HDA_RATE(44, 1, 2);
		break;
	case 32000:
		format = HDA_RATE(48, 2, 3);
		break;
	case 44100:
		format = HDA_RATE(44, 1, 1);
		break;
	case 48000:
		format = HDA_RATE(48, 1, 1);
		break;
	case 88200:
		format = HDA_RATE(44, 2, 1);
		break;
	case 96000:
		format = HDA_RATE(48, 2, 1);
		break;
	case 176400:
		format = HDA_RATE(44, 4, 1);
		break;
	case 192000:
		format = HDA_RATE(48, 4, 1);
		break;
	}

	{
		UINT16 channels = stream->streamFormat.NumberOfChannels;
		if (channels == 0 || channels > 8)
			return 0;
		format |= channels - 1;

		switch (stream->streamFormat.ContainerSize) {
		case 8:
			format |= AC_FMT_BITS_8;
			break;
		case 16:
			format |= AC_FMT_BITS_16;
			break;
		case 20:
			format |= AC_FMT_BITS_20;
			break;
		case 24:
			format |= AC_FMT_BITS_24;
			break;
		case 32:
			format |= AC_FMT_BITS_32;
			break;
		}
	}
	return format;
}

void hdac_stream_setup(PHDAC_STREAM stream) {
	/* make sure the run bit is zero for SD */
	hdac_stream_clear(stream);

	UINT val;
	/* program the stream_tag */
	val = stream_read32(stream, SD_CTL);
	val = (val & ~SD_CTL_STREAM_TAG_MASK) |
		(stream->streamTag << SD_CTL_STREAM_TAG_SHIFT);
	stream_write32(stream, SD_CTL, val);

	/* program the length of samples in cyclic buffer */
	stream_write32(stream, SD_CBL, stream->bufSz);

	/* program the stream format */
	/* this value needs to be the same as the one programmed */
	UINT16 format = hdac_format(stream);
	stream_write16(stream, SD_FORMAT, format);

	/* program the stream LVI (last valid index) of the BDL */
	stream_write16(stream, SD_LVI, stream->numBlocks - 1);

	/* program the BDL address */
	/* lower BDL address */
	PHYSICAL_ADDRESS bdlAddr = MmGetPhysicalAddress(stream->bdl);
	stream_write32(stream, SD_BDLPL, bdlAddr.LowPart);
	/* upper BDL address */
	stream_write32(stream, SD_BDLPU, bdlAddr.HighPart);

	//Enable position buffer
	if (!(hda_read32(stream->FdoContext, DPLBASE) & HDA_DPLBASE_ENABLE)){
		PHYSICAL_ADDRESS posbufAddr = MmGetPhysicalAddress(stream->FdoContext->posbuf);
		hda_write32(stream->FdoContext, DPLBASE, posbufAddr.LowPart | HDA_DPLBASE_ENABLE);
	}

	/* set the interrupt enable bits in the descriptor control register */
	stream_update32(stream, SD_CTL, 0, SD_INT_MASK);

	stream->fifoSize = 0;
	stream->fifoSize = stream_read16(stream, SD_FIFOSIZE) + 1;
}