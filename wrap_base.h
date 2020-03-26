#ifndef WRAP_BASE_H_
#define WRAP_BASE_H_

#ifdef __cplusplus
extern "C"{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

int find_stream_index(AVFormatContext *pformat_ctx, int *video_stream, int *audio_stream);

void get_file_name(char *filename, int argc, char *argv[]);

#ifdef __cplusplus
}
#endif
#endif