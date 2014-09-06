/*
 *  linux/drivers/sound/ezx-audio.h
 *
 *
 *  Description:  audio interface for ezx
 *
 *
 *  Copyright:	BJDC motorola.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *
 *  History:
 *  zhou qiong         June,2002               Created
 *  LiYong             Sep 23,2003             Port from EZX
 *  Jin Lihong(w20076) Jan 02,2004,Libdd66088  (1) Port from UDC e680 kernel of jem vob.
 *                                             (2) Move all audio driver DEBUG macro definition to ezx-audio.h
 *                                                 header file,and redefine DEBUG to EZX_OSS_DEBUG/EZX_OSS_AUDIO_DEBUG
 *                                             (3) reorganize file header
 *  Jin Lihong(w20076) Jan 13,2004,LIBdd68327  Make the e680 louder speaker work.
 *  Jin Lihong(w20076) Feb.23,2004,LIBdd79747  add e680 audio path switch and gain setting funcs
 *  Jin Lihong(w20076) Mar.15,2004,LIBdd86574  mixer bug fix
 *  Jin Lihong(w20076) Apr.13,2004,LIBdd96876  close dsp protection,and add 3d control interface for app
 *
 */

#ifndef EZX_AUDIO_H
#define EZX_AUDIO_H


#define NO_HEADSET	0
#define MONO_HEADSET	1
#define STEREO_HEADSET	2

#define OUT_ADJUST_GAIN		2
#define IN_ADJUST_GAIN		12

#define ASSP_CLK                    (1<<20)
#define ASSP_PINS                   (7<<17)


#define PHONE_CODEC_DEFAULT_RATE    8000
#define PHONE_CODEC_16K_RATE        16000


#define STEREO_CODEC_8K_RATE        8000
#define STEREO_CODEC_11K_RATE       11025
#define STEREO_CODEC_12K_RATE       12000
#define STEREO_CODEC_16K_RATE       16000
#define STEREO_CODEC_22K_RATE       22050
#define STEREO_CODEC_24K_RATE       24000
#define STEREO_CODEC_32K_RATE       32000
#define STEREO_CODEC_44K_RATE       44100
#define STEREO_CODEC_48K_RATE       48000

#endif

