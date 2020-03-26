#define __STDC_CONSTANT_MACROS      //ffmpeg要求

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

#include <assert.h>
#include "wrap_base.h"
#ifdef __cplusplus
}
#endif

 
int find_stream_index(AVFormatContext *pformat_ctx, int *video_stream, int *audio_stream)
{
    assert(video_stream != NULL || audio_stream != NULL);

     int i = 0;
     int audio_index = -1;
     int video_index = -1;

     for (i = 0; i < pformat_ctx->nb_streams; i++)
     {
          if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
          {
              video_index = i;
          }
          if (pformat_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
          {
              audio_index = i;
          }
     }

   
     if (video_stream == NULL)
     {
         *audio_stream = audio_index;
         return *audio_stream;
     }

     if (audio_stream == NULL)
     {
         *video_stream = video_index;
          return *video_stream;
     }

     *video_stream = video_index;
     *audio_stream = audio_index;

     return 0;
}

 
void get_file_name(char *filename, int argc, char *argv[])
{
    if (argc == 2)
    {
        memcpy(filename, argv[1], strlen(argv[1])+1);
    }
    else if (argc > 2)
    {
         fprintf(stderr, "Usage: ./*.out filename\n");
         exit(-1);
    }
}