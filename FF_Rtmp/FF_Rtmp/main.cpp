//
//  main.cpp
//  FF_Rtmp
//
//  Created by 叶亮 on 2019/4/29.
//  Copyright © 2019 叶亮. All rights reserved.
//

extern "C"{
#include "libavformat/avformat.h"
#include "libavutil/time.h"
}

#include <iostream>

#pragma comment(lib, "avformat.lib")

using namespace std;

int printError(int errorNum)
{
    char buf[1024] = {0};
    av_strerror(errorNum, buf, sizeof(buf));
    cout << buf << endl;
    return -1;
}

int main(int argc, char *argv[])
{
    char  *inUrl = "/Users/yeliang/Desktop/video_data/test.flv";
    char  *outUrl = "rtmp://localhost/live";
    
    //初始化所有封装和解封装 flv mp4 mov mp3
    av_register_all();
    
    //网络库初始化
    avformat_network_init();
    
    //1 打开文件，解封装
    AVFormatContext *inFormatCtx = NULL;
    
    int re = avformat_open_input(&inFormatCtx, inUrl, 0, 0);
    
    if(re != 0){
        return printError(re);
    }
    cout << "open file " << inUrl << " Sucess!" << endl;
    
    //获取音频视频流信息 h264 flv
    re = avformat_find_stream_info(inFormatCtx,0);
    if(re != 0){
        return printError(re);
    }
    
    av_dump_format(inFormatCtx, 0, inUrl, 0);
    
    //以下是打印的音频和视频流信息(mp4的)
    //    Input #0, mov,mp4,m4a,3gp,3g2,mj2, from '/Users/yeliang/Desktop/video_data/video.mp4':
    //        Metadata:
    //            major_brand     : isom
    //            minor_version   : 512
    //        compatible_brands: isomiso2avc1mp41
    //            encoder         : Lavf58.20.100
    //        Duration: 00:00:10.02, start: 0.000000, bitrate: 1973 kb/s
    //            Stream #0:0(eng): Video: h264 (High) (avc1 / 0x31637661), yuv420p, 1920x1080 [SAR 1:1 DAR 16:9], 1842 kb/s, 25 fps, 25 tbr, 12800 tbn, 50 tbc (default)
    //        Metadata:
    //            handler_name    : VideoHandler
    //            Stream #0:1(eng): Audio: aac (LC) (mp4a / 0x6134706D), 44100 Hz, stereo, fltp, 128 kb/s (default)
    //        Metadata:
    //            handler_name    : SoundHandler
    
    
    //====================================================
    //输出流
    AVFormatContext *outFormatCtx = NULL;
    re = avformat_alloc_output_context2(&outFormatCtx, 0, "flv", outUrl);
    if(!outFormatCtx)
    {
        return printError(re);
    }
    cout << "outFormatCtx Create Success! " <<endl;
    
    //配置输出流
    //遍历输入的AVStream
    for(int i = 0; i < inFormatCtx->nb_streams; i++){
        AVStream *out = avformat_new_stream(outFormatCtx, inFormatCtx->streams[i]->codec->codec);
        if(!out)
        {
            return printError(0);
        }
        //复制配置信息
        //re = avcodec_copy_context(out->codec, inFormatCtx->streams[i]->codec);
        re = avcodec_parameters_copy(out->codecpar, inFormatCtx->streams[i]->codecpar);
        out->codec->codec_tag = 0;
    }
    
    av_dump_format(outFormatCtx, 0, outUrl, 1);
    //打印的s输出流信息
    //    Stream #0:0: Audio: mp3, 44100 Hz, stereo, fltp, 128 kb/s
    //    Stream #0:1: Video: flv1, yuv420p, 1920x1080, q=2-31, 200 kb/s
    
    //======================================================
    //rtmp 推流
    
    //打开io
    re = avio_open(&outFormatCtx->pb, outUrl, AVIO_FLAG_WRITE);
    if(!outFormatCtx->pb){
        return printError(re);
    }
    
    //写入头信息
    re = avformat_write_header(outFormatCtx, 0);
    if(re < 0)
    {
        return printError(re);
    }
    
    cout << "avformat_write_header Success！ "<< re << endl;
    
    //推流每一帧数据
    AVPacket pkt;
    
    for(;;)
    {
        re = av_read_frame(inFormatCtx, &pkt);
        if(re != 0){
            break;
        }
        cout << pkt.pts << "   "<< flush;
        
        //计算转换pts dts
        AVRational inTime = inFormatCtx->streams[pkt.stream_index]->time_base;
        AVRational outTime = outFormatCtx->streams[pkt.stream_index]->time_base;
        
        pkt.pts = av_rescale_q_rnd(pkt.pts, inTime, outTime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        pkt.dts = av_rescale_q_rnd(pkt.pts, inTime, outTime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        
        pkt.duration = av_rescale_q_rnd(pkt.duration, inTime, outTime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_NEAR_INF));
        pkt.pos = -1;
        
        if(pkt.stream_index == 0){
            av_usleep(30*1000);
        }
        
        re = av_interleaved_write_frame(outFormatCtx, &pkt);
        
        if(re < 0){
            return printError(re);
        }
        
    }
    getchar();
    return 0;
}

