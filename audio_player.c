#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <io.h>

 
#if _MSC_VER>=1900
#include "stdio.h" 
_ACRTIMP_ALT FILE* __cdecl __acrt_iob_func(unsigned);
#ifdef __cplusplus 
extern "C"
#endif 
FILE* __cdecl __iob_func(unsigned i) {
	return __acrt_iob_func(i);
}
#endif /* _MSC_VER>=1900 */


#define __STDC_CONSTANT_MACROS
 

#ifdef __cplusplus
extern "C"
{
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>

#ifdef __cplusplus
}
#endif


//
#include "wrap_base.h"
#include "packet_queue.h"

#define AVCODE_MAX_AUDIO_FRAME_SIZE    192000  //1 second of 48khz 32bit audio
#define SDL_AUDIO_BUFFER_SIZE   1024    //

#define FILE_NAME               "desk.aac"
#define ERR_STREAM              stderr
#define OUT_SAMPLE_RATE         44100

AVFrame         wanted_frame;
PacketQueue     audio_queue;
int      quit = 0;

void audio_callback(void *userdata, Uint8 *stream, int len);
int audio_decode_frame(AVCodecContext *pcodec_ctx, uint8_t *audio_buf, int buf_size);
 
int main(int argc, char *argv[])
{
	AVFormatContext     *pformat_ctx = NULL;
	int                 audio_stream = -1;
	AVCodecContext      *pcodec_ctx = NULL;
	AVCodecContext      *pcodec_ctx_cp = NULL;
	AVCodec             *pcodec = NULL;
	AVPacket            packet;        //!
	AVFrame             *pframe = NULL;
	char                filename[256] = FILE_NAME;

	//SDL
	SDL_AudioSpec       wanted_spec;
	SDL_AudioSpec       spec;

	//ffmpeg 初始化
	av_register_all();

	//SDL初始化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		fprintf(ERR_STREAM, "Couldn't init SDL:%s\n", SDL_GetError());
		exit(-1);
	}

	get_file_name(filename, argc, argv);

	//打开文件
	if (avformat_open_input(&pformat_ctx, filename, NULL, NULL) != 0)
	{
		fprintf(ERR_STREAM, "Couldn't open input file\n");
		exit(-1);
	}

 
	if (avformat_find_stream_info(pformat_ctx, NULL) < 0)
	{
		fprintf(ERR_STREAM, "Not Found Stream Info\n");
		exit(-1);
	}

	//显示文件信息，十分好用的一个函数
	av_dump_format(pformat_ctx, 0, filename, 0);

 
	if (find_stream_index(pformat_ctx, NULL, &audio_stream) == -1)
	{
		fprintf(ERR_STREAM, "Couldn't find stream index\n");
		exit(-1);
	}
	printf("audio_stream = %d\n", audio_stream);
 
	pcodec_ctx = pformat_ctx->streams[audio_stream]->codec;
	pcodec = avcodec_find_decoder(pcodec_ctx->codec_id);
	if (!pcodec)
	{
		fprintf(ERR_STREAM, "Couldn't find decoder\n");
		exit(-1);
	}
 
 
	wanted_spec.freq = pcodec_ctx->sample_rate;
	wanted_spec.format = AUDIO_S16SYS;
	wanted_spec.channels = pcodec_ctx->channels;        //通道数
	wanted_spec.silence = 0;    //设置静音值
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;       
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = pcodec_ctx;
 
	if (SDL_OpenAudio(&wanted_spec, &spec) < 0)
	{
		fprintf(ERR_STREAM, "Couldn't open Audio:%s\n", SDL_GetError());
		exit(-1);
	}

 
	wanted_frame.format = AV_SAMPLE_FMT_S16;
	wanted_frame.sample_rate = spec.freq;
	wanted_frame.channel_layout = av_get_default_channel_layout(spec.channels);
	wanted_frame.channels = spec.channels;

 
	avcodec_open2(pcodec_ctx, pcodec, NULL);
 
	packet_queue_init(&audio_queue);
 
	SDL_PauseAudio(0);          

	//读一帧数据
	while (av_read_frame(pformat_ctx, &packet) >= 0)     //读一个packet，数据放在packet.data
	{
		if (packet.stream_index == audio_stream)
		{
			packet_queue_put(&audio_queue, &packet);
		}
		else
		{
			av_free_packet(&packet);
		}
	}
	getchar();      //...
	return 0;
}

 
void audio_callback(void *userdata, Uint8 *stream, int len)
{
	AVCodecContext *pcodec_ctx = (AVCodecContext *)userdata;
	int len1 = 0;
	int audio_size = 0;
 
	static uint8_t         audio_buf[(AVCODE_MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int    audio_buf_size = 0;
	static unsigned int    audio_buf_index = 0;

 
	SDL_memset(stream, 0, len);

	while (len > 0)
	{
		if (audio_buf_index >= audio_buf_size)
		{
	 
			audio_size = audio_decode_frame(pcodec_ctx, audio_buf, sizeof(audio_buf));
			if (audio_size < 0)
			{
			 
				audio_buf_size = 1024;
				memset(audio_buf, 0, audio_buf_size);
			}
			else
			{
				audio_buf_size = audio_size;
			}
			audio_buf_index = 0;      
		}

		len1 = audio_buf_size - audio_buf_index;
 
		if (len1 > len)      
		{
			len1 = len;
		}
 
		SDL_MixAudio(stream, (uint8_t*)audio_buf + audio_buf_index, len, SDL_MIX_MAXVOLUME);
		//
		//memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}

}

 
int audio_decode_frame(AVCodecContext *pcodec_ctx,
	uint8_t *audio_buf, int buf_size)
{
	AVPacket   packet;
	AVFrame    *frame;
	int        got_frame;
	int        pkt_size = 0;
	//     uint8_t    *pkt_data = NULL;
	int        decode_len;
	int        try_again = 0;
	long long  audio_buf_index = 0;
	long long  data_size = 0;
	SwrContext *swr_ctx = NULL;
	int        convert_len = 0;
	int        convert_all = 0;

	if (packet_queue_get(&audio_queue, &packet, 1) < 0)
	{
		fprintf(ERR_STREAM, "Get queue packet error\n");
		return -1;
	}

	//     pkt_data = packet.data;
	pkt_size = packet.size;
	//     fprintf(ERR_STREAM, "pkt_size = %d\n", pkt_size);

	frame = av_frame_alloc();
	while (pkt_size > 0)
	{
 
		decode_len = avcodec_decode_audio4(pcodec_ctx,
			frame, &got_frame, &packet);
		if (decode_len < 0)  
		{
 
			fprintf(ERR_STREAM, "Couldn't decode frame\n");
			if (try_again == 0)
			{
				try_again++;
				continue;
			}
			try_again = 0;
		}


		if (got_frame)
		{
	  
			if (frame->channels > 0 && frame->channel_layout == 0)
			{
			 
				frame->channel_layout = av_get_default_channel_layout(frame->channels);
			}
			else if (frame->channels == 0 && frame->channel_layout > 0)
			{
				frame->channels = av_get_channel_layout_nb_channels(frame->channel_layout);
			}


			if (swr_ctx != NULL)
			{
				swr_free(&swr_ctx);
				swr_ctx = NULL;
			}

		 
			swr_ctx = swr_alloc_set_opts(NULL,
				wanted_frame.channel_layout, wanted_frame.format, wanted_frame.sample_rate,
				frame->channel_layout,
				frame->format, frame->sample_rate, 0, NULL);
			 
			if (swr_ctx == NULL || swr_init(swr_ctx) < 0)
			{
				fprintf(ERR_STREAM, "swr_init error\n");
				break;
			}
	 
			convert_len = swr_convert(swr_ctx,
				&audio_buf + audio_buf_index,
				AVCODE_MAX_AUDIO_FRAME_SIZE,
				(const uint8_t **)frame->data,
				frame->nb_samples);

			///printf("decode len = %d, convert_len = %d\n", decode_len, convert_len);
	 
			pkt_size -= decode_len;
		 
			audio_buf_index += convert_len;//data_size;
 
			convert_all += convert_len;
		}
	}
	return wanted_frame.channels * convert_all * av_get_bytes_per_sample(wanted_frame.format);
	 
}