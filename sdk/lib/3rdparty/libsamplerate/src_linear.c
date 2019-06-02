/*
** Copyright (c) 2002-2016, Erik de Castro Lopo <erikd@mega-nerd.com>
** All rights reserved.
**
** This code is released under 2-clause BSD license. Please see the
** file at : https://github.com/erikd/libsamplerate/blob/master/COPYING
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "float_cast.h"
#include "common.h"

static int linear_vari_process (SRC_PRIVATE *psrc, SRC_DATA *data) ;
static void linear_reset (SRC_PRIVATE *psrc) ;
static int linear_copy (SRC_PRIVATE *from, SRC_PRIVATE *to) ;

/*========================================================================================
*/

#define	LINEAR_MAGIC_MARKER	MAKE_MAGIC ('l', 'i', 'n', 'e', 'a', 'r')

#define	SRC_DEBUG	0

typedef struct
{	int		linear_magic_marker ;
	int		channels ;
	int		reset ;
	long	in_count, in_used ;
	long	out_count, out_gen ;
	float	last_value [1] ;
} LINEAR_DATA ;

/*----------------------------------------------------------------------------------------
*/

static int
linear_vari_process (SRC_PRIVATE *psrc, SRC_DATA *data)
{	LINEAR_DATA *priv ;
	double		src_ratio, input_index, rem ;
	int			ch ;

	if (data->input_frames <= 0)
		return SRC_ERR_NO_ERROR ;

	if (psrc->private_data == NULL)
		return SRC_ERR_NO_PRIVATE ;

	priv = (LINEAR_DATA*) psrc->private_data ;

	if (priv->reset)
	{	/* If we have just been reset, set the last_value data. */
		for (ch = 0 ; ch < priv->channels ; ch++)
			priv->last_value [ch] = data->data_in [ch] ;
		priv->reset = 0 ;
		} ;

	priv->in_count = data->input_frames * priv->channels ;
	priv->out_count = data->output_frames * priv->channels ;
	priv->in_used = priv->out_gen = 0 ;

	src_ratio = psrc->last_ratio ;

	if (is_bad_src_ratio (src_ratio))
		return SRC_ERR_BAD_INTERNAL_STATE ;

	input_index = psrc->last_position ;

	/* Calculate samples before first sample in input array. */
	while (input_index < 1.0 && priv->out_gen < priv->out_count)
	{
		if (priv->in_used + priv->channels * (1.0 + input_index) >= priv->in_count)
			break ;

		if (priv->out_count > 0 && fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + priv->out_gen * (data->src_ratio - psrc->last_ratio) / priv->out_count ;

		for (ch = 0 ; ch < priv->channels ; ch++)
		{	data->data_out [priv->out_gen] = (float) (priv->last_value [ch] + input_index *
										(data->data_in [ch] - priv->last_value [ch])) ;
			priv->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		} ;

	rem = fmod_one (input_index) ;
	priv->in_used += priv->channels * lrint (input_index - rem) ;
	input_index = rem ;

	/* Main processing loop. */
	while (priv->out_gen < priv->out_count && priv->in_used + priv->channels * input_index < priv->in_count)
	{
		if (priv->out_count > 0 && fabs (psrc->last_ratio - data->src_ratio) > SRC_MIN_RATIO_DIFF)
			src_ratio = psrc->last_ratio + priv->out_gen * (data->src_ratio - psrc->last_ratio) / priv->out_count ;

		if (SRC_DEBUG && priv->in_used < priv->channels && input_index < 1.0)
		{	printf ("Whoops!!!!   in_used : %ld     channels : %d     input_index : %f\n", priv->in_used, priv->channels, input_index) ;
			exit (1) ;
			} ;

		for (ch = 0 ; ch < priv->channels ; ch++)
		{	data->data_out [priv->out_gen] = (float) (data->data_in [priv->in_used - priv->channels + ch] + input_index *
						(data->data_in [priv->in_used + ch] - data->data_in [priv->in_used - priv->channels + ch])) ;
			priv->out_gen ++ ;
			} ;

		/* Figure out the next index. */
		input_index += 1.0 / src_ratio ;
		rem = fmod_one (input_index) ;

		priv->in_used += priv->channels * lrint (input_index - rem) ;
		input_index = rem ;
		} ;

	if (priv->in_used > priv->in_count)
	{	input_index += (priv->in_used - priv->in_count) / priv->channels ;
		priv->in_used = priv->in_count ;
		} ;

	psrc->last_position = input_index ;

	if (priv->in_used > 0)
		for (ch = 0 ; ch < priv->channels ; ch++)
			priv->last_value [ch] = data->data_in [priv->in_used - priv->channels + ch] ;

	/* Save current ratio rather then target ratio. */
	psrc->last_ratio = src_ratio ;

	data->input_frames_used = priv->in_used / priv->channels ;
	data->output_frames_gen = priv->out_gen / priv->channels ;

	return SRC_ERR_NO_ERROR ;
} /* linear_vari_process */

/*------------------------------------------------------------------------------
*/

const char*
linear_get_name (int src_enum)
{
	if (src_enum == SRC_LINEAR)
		return "Linear Interpolator" ;

	return NULL ;
} /* linear_get_name */

const char*
linear_get_description (int src_enum)
{
	if (src_enum == SRC_LINEAR)
		return "Linear interpolator, very fast, poor quality." ;

	return NULL ;
} /* linear_get_descrition */

int
linear_set_converter (SRC_PRIVATE *psrc, int src_enum)
{	LINEAR_DATA *priv = NULL ;

	if (src_enum != SRC_LINEAR)
		return SRC_ERR_BAD_CONVERTER ;

	if (psrc->private_data != NULL)
	{	free (psrc->private_data) ;
		psrc->private_data = NULL ;
		} ;

	if (psrc->private_data == NULL)
	{	priv = calloc (1, sizeof (*priv) + psrc->channels * sizeof (float)) ;
		psrc->private_data = priv ;
		} ;

	if (priv == NULL)
		return SRC_ERR_MALLOC_FAILED ;

	priv->linear_magic_marker = LINEAR_MAGIC_MARKER ;
	priv->channels = psrc->channels ;

	psrc->const_process = linear_vari_process ;
	psrc->vari_process = linear_vari_process ;
	psrc->reset = linear_reset ;
	psrc->copy = linear_copy ;

	linear_reset (psrc) ;

	return SRC_ERR_NO_ERROR ;
} /* linear_set_converter */

/*===================================================================================
*/

static void
linear_reset (SRC_PRIVATE *psrc)
{	LINEAR_DATA *priv = NULL ;

	priv = (LINEAR_DATA*) psrc->private_data ;
	if (priv == NULL)
		return ;

	priv->channels = psrc->channels ;
	priv->reset = 1 ;
	memset (priv->last_value, 0, sizeof (priv->last_value [0]) * priv->channels) ;

	return ;
} /* linear_reset */

static int
linear_copy (SRC_PRIVATE *from, SRC_PRIVATE *to)
{
	if (from->private_data == NULL)
		return SRC_ERR_NO_PRIVATE ;

	LINEAR_DATA *to_priv = NULL ;
	LINEAR_DATA* from_priv = (LINEAR_DATA*) from->private_data ;
	size_t private_size = sizeof (*to_priv) + from_priv->channels * sizeof (float) ;

	if ((to_priv = calloc (1, private_size)) == NULL)
		return SRC_ERR_MALLOC_FAILED ;

	memcpy (to_priv, from_priv, private_size) ;
	to->private_data = to_priv ;

	return SRC_ERR_NO_ERROR ;
} /* linear_copy */
