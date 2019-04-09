#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <SDL2/SDL.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
SDL_Texture* sdlTexture;
SDL_Renderer* sdlRenderer;

int thread_exit=0;

#define REFRESH_EVENT  (SDL_USEREVENT + 1)
int refresh_video(void *opaque)
{
    SDL_Rect sdlRect;
    int len = 176*144 + ((176*144) >> 1);
    unsigned char *buffer = malloc(sizeof(char)*len);

    const char *input = "test.264";

    //1.注册所有组件
    av_register_all();

    //封装格式上下文，统领全局的结构体，保存了视频文件封装格式的相关信息
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    //2.打开输入视频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0)
    {
        printf("%s", "无法打开输入视频文件");
        return -1;
    }

    //3.获取视频文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("%s", "无法获取视频文件信息");
        return -1;
    }

    //获取视频流的索引位置
    //遍历所有类型的流（音频流、视频流、字幕流），找到视频流
    int v_stream_idx = -1;
    int i = 0;
    //number of streams
    for (; i < pFormatCtx->nb_streams; i++)
    {
        //流的类型
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            v_stream_idx = i;
            break;
        }
    }

    if (v_stream_idx == -1)
    {
        printf("%s", "找不到视频流\n");
        return -1;
    }

    //只有知道视频的编码方式，才能够根据编码方式去找到解码器
    //获取视频流中的编解码上下文
    AVCodecContext *pCodecCtx = pFormatCtx->streams[v_stream_idx]->codec;
    //4.根据编解码上下文中的编码id查找对应的解码
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        printf("%s", "找不到解码器\n");
        return -1;
    }

    //5.打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("%s", "解码器无法打开\n");
        return -1;
    }

    //输出视频信息
    printf("视频的文件格式：%s", pFormatCtx->iformat->name);
    printf("视频时长：%d", (pFormatCtx->duration) / 1000000);
    printf("视频的宽高：%d,%d", pCodecCtx->width, pCodecCtx->height);
    printf("解码器的名称：%s", pCodec->name);

    //准备读取
    //AVPacket用于存储一帧一帧的压缩数据（H264）
    //缓冲区，开辟空间
    AVPacket *packet = (AVPacket*)av_malloc(sizeof(AVPacket));

    //AVFrame用于存储解码后的像素数据(YUV)
    //内存分配
    AVFrame *pFrame = av_frame_alloc();
    //YUV420
    AVFrame *pFrameYUV = av_frame_alloc();
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    //初始化缓冲区
    avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

    //用于转码（缩放）的参数，转之前的宽高，转之后的宽高，格式等
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
        pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,
        SWS_BICUBIC, NULL, NULL, NULL);
    int got_picture, ret;

    while (thread_exit == 0 && av_read_frame(pFormatCtx, packet) >= 0)
    {
        //只要视频压缩数据（根据流的索引位置判断）
        if (packet->stream_index == v_stream_idx)
        {
            //7.解码一帧视频压缩数据，得到视频像素数据
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if (ret < 0)
            {
                printf("%s", "解码错误");
                return -1;
            }

            //为0说明解码完成，非0正在解码
            if (got_picture)
            {
                //AVFrame转为像素格式YUV420，宽高
                //2 6输入、输出数据
                //3 7输入、输出画面一行的数据的大小 AVFrame 转换是一行一行转换的
                //4 输入数据第一列要转码的位置 从0开始
                //5 输入画面的高度
                sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                    pFrameYUV->data, pFrameYUV->linesize);

                //输出到YUV文件
                //AVFrame像素帧写入文件
                //data解码后的图像像素数据（音频采样数据）
                //Y 亮度 UV 色度（压缩了） 人对亮度更加敏感
                //U V 个数是Y的1/4
                int y_size = pCodecCtx->width * pCodecCtx->height;

                memcpy(buffer, pFrameYUV->data[0], y_size);
                memcpy(buffer + y_size, pFrameYUV->data[1], y_size / 4);
                memcpy(buffer + y_size + y_size / 4, pFrameYUV->data[2], y_size / 4);

                SDL_UpdateTexture( sdlTexture, NULL, buffer, 176);

                //FIX: If window is resize
                sdlRect.x = 0;
                sdlRect.y = 0;
                sdlRect.w = 176;
                sdlRect.h = 144;

                //SDL_RenderClear( sdlRenderer );
                SDL_RenderCopy( sdlRenderer, sdlTexture, NULL, &sdlRect);
                SDL_RenderPresent( sdlRenderer );
            }
        }

        //释放资源
        av_free_packet(packet);
        Sleep(30);
    }

    free(buffer);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pFormatCtx);

    return 0;
}

int main(int argc, char **args)
{
    SDL_Window* window = NULL;

    if (SDL_Init( SDL_INIT_VIDEO ) < 0)
    {
        printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow( "H264Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 176, 144, SDL_WINDOW_SHOWN );
    if (window == NULL)
    {
        printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
        SDL_Quit();
        return -1;
    }

    sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 176, 144);

    SDL_Thread *refresh_thread = SDL_CreateThread(refresh_video,NULL,NULL);
    SDL_Event event;

    while(1){
        SDL_WaitEvent(&event);
        if(event.type==REFRESH_EVENT){

        }else if(event.type == SDL_WINDOWEVENT){
            //If Resize
            //SDL_GetWindowSize(screen,&screen_w,&screen_h);
        }else if(event.type == SDL_QUIT){
            thread_exit = 1;
            break;
        }
    }

    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow( window );
    SDL_Quit();

    return 0;
}
