//---------------------------------------------------------------------------
// SMS Plus / EZX
// Sega Master System / Game Gear emulator for EZX platform
// Based on SMS Plus by Charles MacDonald
// EZX version ported by Lin Fan (cnlinfan@gmail.com)
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

#define CONFIG_ARCH_EZX_E680
#define MAKE_FTR_HAPTICS

typedef unsigned int u32;
typedef unsigned char u8;

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "soundcard.h"
#include "ezx-common.h"
#include "ezx-audio.h"
#include "shared.h"
#include "qsmsplus.h"
#include "qmainapp.h"

sp_option option;
static char *game_name;
static unsigned char *bmpdata;

int kst_up, kst_down, kst_left, kst_right, kst_a, kst_b, kst_start;

int dsp_dev, mixer_dev;

int exit_flag	= 0;

float vol_factor = 1.0;
float vol_factor_table[] = {0.0, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 2.5, 3.0, 4.0, 5.0};

int16 *sound_buffer;

#define SOUND_DEVICE		"/dev/dsp"
#define SOUND_SAMPLERATE	22050

static int audio_activate(void)
{
	int status, hstest, location = LOUDERSPEAKER_OUT;

	mixer_dev = open("/dev/mixer", O_RDONLY);
	if(mixer_dev == -1)
		return 0;

	ioctl(mixer_dev, SOUND_MIXER_READ_HEADSET_STATUS, &hstest);
	switch(hstest)
	{
		case MONO_HEADSET:
			location = HEADJACK_OUT; // Warning!! untested
			break;
		case STEREO_HEADSET:
			location = HEADSET_OUT;
			break;
	}
	status = ioctl(mixer_dev, SOUND_MIXER_WRITE_OUTSRC, &location);
	if(status == -1)
		return 0;

    return 1;
}

void audio_setvolume(int vol)
{
	if (vol < 0) vol = 0;
	else if (vol > 10) vol = 10;
	vol_factor = vol_factor_table[vol];
}

static int audio_init(void)
{
	if ((dsp_dev = open(SOUND_DEVICE, O_WRONLY, 0)) == -1)
		return 0;
	
	int soundp_format = AFMT_S16_LE;
	if (ioctl(dsp_dev, SNDCTL_DSP_SETFMT, &soundp_format) == -1)
		return 0;

	int soundp_stereo = 1;
	if (ioctl(dsp_dev, SNDCTL_DSP_STEREO, &soundp_stereo) == -1)
		return 0;

	int soundp_speed = SOUND_SAMPLERATE;
	if (ioctl(dsp_dev, SNDCTL_DSP_SPEED, &soundp_speed) == -1)
		return 0;

	audio_activate();
	audio_setvolume(option.volume);
	return 1;
}

static void audio_uninit(void)
{
	if (dsp_dev != -1)
		close(dsp_dev);
	dsp_dev = -1;
	
	if (mixer_dev != -1)
		close(mixer_dev);
	mixer_dev = -1;
}

static int machine_init()
{
	bmpdata = malloc(SMS_WIDTH * SMS_HEIGHT * 2);
	memset(&bitmap, 0, sizeof(bitmap_t));
	bitmap.width  = SMS_WIDTH;
	bitmap.height = SMS_HEIGHT;
	bitmap.depth  = 16;
	bitmap.pitch  = bitmap.width * 2;
	bitmap.data = bmpdata;

	if (!option.mute) {
		if (!audio_init()) {
			printf("Warning: can not open audio device.\n");
			option.mute = 1;
			audio_uninit();
		}
	}

	return 1;
}

static void machine_uninit()
{
	audio_uninit();
	free(bmpdata);
	return;
}

void update_input(void)
{
	/* Clear all button states */
	memset(&input, 0, sizeof(input_t));
	
	/* System buttons */
	if (kst_start) input.system |= (IS_GG) ? INPUT_START : INPUT_PAUSE;

	/* Player 1 buttons */
	if (kst_up)
		input.pad[0] |= INPUT_UP;
	else if (kst_down)
		input.pad[0] |= INPUT_DOWN;
	if (kst_left)
		input.pad[0] |= INPUT_LEFT;
	else if (kst_right)
		input.pad[0] |= INPUT_RIGHT;
	if (kst_a)
		input.pad[0] |= INPUT_BUTTON1;
	if (kst_b)
		input.pad[0] |= INPUT_BUTTON2;
}

void ezx_sound_mixer_callback(int16 **stream, int16 **output, int length)
{
	int i;
	int16 temp;
	for(i = 0; i < length; i++)
	{
		temp = (stream[STREAM_FM_MO][i] + stream[STREAM_FM_RO][i]) >> 1;
		sound_buffer[i * 2] = (int)((temp + stream[STREAM_PSG_L][i]) * vol_factor);
		sound_buffer[i * 2 + 1] = (int)((temp + stream[STREAM_PSG_R][i]) * vol_factor);
	}
}

void update_audio(void)
{
	if ((!option.mute) && (option.volume > 0))
		write(dsp_dev, sound_buffer, snd.buffer_size * 2);
}

int smsplus_init()
{
	game_name = option.game_name;
	if(load_rom(game_name) == 0) {
		printf("ERROR: can't load '%s'.\n", game_name);
		return 0;
	}
	else
		printf("Loaded '%s'.\n", game_name);
	
	printf("Initializing EZX ... ");
	if (!machine_init())
		return 0;
	printf("Ok.\n");

	printf("Initializing Emulation ... ");
	snd.fm_which = SND_EMU2413;
	snd.fps = (sms.display == DISPLAY_NTSC) ? FPS_NTSC : FPS_PAL;
	snd.fm_clock = (sms.display == DISPLAY_NTSC) ? CLOCK_NTSC : CLOCK_PAL;
	snd.psg_clock = (sms.display == DISPLAY_NTSC) ? CLOCK_NTSC : CLOCK_PAL;
	snd.sample_rate = option.mute ? 0 : SOUND_SAMPLERATE;
	snd.mixer_callback = ezx_sound_mixer_callback;
	system_init();
	sms.territory = option.country;
	sms.use_fm = option.enablefm;
	sound_buffer = malloc(snd.buffer_size * 2);
    	system_poweron();	
	printf("Ok.\n");
	
	return 1;
}

void smsplus_emulate()
{
	/* Main emulation loop */
	for(;;)
	{
		if(exit_flag) break;
		
		system_frame(0);
		
		update_input();
		update_audio();
		update_video();
	}
}

void smsplus_uninit() 
{
	free(sound_buffer);

	printf("Shutting Down Emulation ...\n");
	system_poweroff();
	system_shutdown();
	
	printf("Shutting Down EZX ...\n");
	machine_uninit();
}

/* Load system state */
int load_state(void)
{
    char name[PATH_MAX];
    FILE *fd = NULL;
    strcpy(name, game_name);
    strcpy(strrchr(name, '.'), ".sav");
    fd = fopen(name, "rb");
    if (!fd) return 0;
    system_load_state(fd);
    fclose(fd);
    return 1;
}

/* Save system state */
int save_state(void)
{
    char name[PATH_MAX];
    FILE *fd = NULL;
    strcpy(name, game_name);
    strcpy(strrchr(name, '.'), ".sav");
    fd = fopen(name, "wb");
    if (!fd) return 0;
    system_save_state(fd);
    fclose(fd);
    return 1;
}

/* Save or load SRAM */
void system_manage_sram(uint8 *sram, int slot, int mode)
{
    char name[PATH_MAX];
    FILE *fd;
    strcpy(name, game_name);
    strcpy(strrchr(name, '.'), ".srm");

    switch(mode)
    {
        case SRAM_SAVE:
            if(sms.save)
            {
                fd = fopen(name, "wb");
                if(fd)
                {
                    fwrite(sram, 0x8000, 1, fd);
                    fclose(fd);
                }
            }
            break;

        case SRAM_LOAD:
            fd = fopen(name, "rb");
            if(fd)
            {
                sms.save = 1;
                fread(sram, 0x8000, 1, fd);
                fclose(fd);
            }
            else
            {
                /* No SRAM file, so initialize memory */
                memset(sram, 0x00, 0x8000);
            }
            break;
    }
}
