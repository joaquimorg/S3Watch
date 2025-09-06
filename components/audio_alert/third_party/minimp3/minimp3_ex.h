#ifndef MINIMP3_EXT_H
#define MINIMP3_EXT_H
/*
    https://github.com/lieff/minimp3
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <stddef.h>
#include "minimp3.h"

/* flags for mp3dec_ex_open_* functions */
#define MP3D_SEEK_TO_BYTE   0      /* mp3dec_ex_seek seeks to byte in stream */
#define MP3D_SEEK_TO_SAMPLE 1      /* mp3dec_ex_seek precisely seeks to sample using index (created during duration calculation scan or when mp3dec_ex_seek called) */
#define MP3D_DO_NOT_SCAN    2      /* do not scan whole stream for duration if vbrtag not found, mp3dec_ex_t::samples will be filled only if mp3dec_ex_t::vbr_tag_found == 1 */
#ifdef MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#define MP3D_ALLOW_MONO_STEREO_TRANSITION  4
#define MP3D_FLAGS_MASK 7
#else
#define MP3D_FLAGS_MASK 3
#endif

typedef struct
{
    mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    int flags, seek_method;
    int64_t samples, offset, start_offset, end_offset, duration;
    int vbr_tag_found;
    int free_format_bytes;
    int last_error;
    int16_t buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
    void *io; /* internal */
} mp3dec_ex_t;

int mp3dec_ex_open_file(mp3dec_ex_t *dec, const char *file, int flags);
void mp3dec_ex_close(mp3dec_ex_t *dec);
size_t mp3dec_ex_read(mp3dec_ex_t *dec, int16_t *buf, size_t samples);

#endif

