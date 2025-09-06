#ifndef MINIMP3_H
#define MINIMP3_H
/*
    https://github.com/lieff/minimp3
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stdint.h>

#define MINIMP3_MAX_SAMPLES_PER_FRAME (1152*2)

typedef struct
{
    int frame_bytes, frame_offset, channels, hz, layer, bitrate_kbps;
} mp3dec_frame_info_t;

typedef struct
{
    float mdct_overlap[2][9*32], qmf_state[15*2*32];
    uint32_t reserv, free_format_bytes;
    int16_t last_bitreservoir;
    uint8_t maindata[1940];
    int md_len, frame_size, part_23_len, main_data_begin, gr_info, scfsi, decode_padding, frame_bytes;
    int hr, hs, gr, ch, ls, lsf, mpeg25, mode, mode_ext, jsbound, start, end, sfreq, tab_offset,
        layer, bitrate_kbps, channels, hz, frame_offset;
} mp3dec_t;

void mp3dec_init(mp3dec_t *dec);
int mp3dec_decode_frame(mp3dec_t *dec, const uint8_t *mp3, int mp3_bytes, int16_t *pcm, mp3dec_frame_info_t *info);

#endif

