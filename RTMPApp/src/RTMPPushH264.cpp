/*
 * RTMPPushH264.cpp
 *
 *  Created on: Jan 12, 2017
 *      Author: tla001
 */

#include "RTMPPushH264.h"
#include "librtmp/log.h"

int runflag=0;
static void sig_user(int signo){
    if(signo==SIGINT){
    	runflag=0;
        printf("received SIGINT\n");
    }
}

void pushYUVByH264(){

	char url[]="rtmp://localhost/live/test1";
	int width=640;
	int height=360;
	int outSize=1024;
	int baseFrameSize=width*height;
	const long bufferSize=baseFrameSize*3;
	char buffer[bufferSize];

	int fps=25;
	int rate=400;

	char *frame=NULL;

    if(signal(SIGINT,sig_user)==SIG_ERR)
        perror("catch SIGINT err");

	FILE* fp  = fopen("test_640x360_yuv420p.yuv", "rb");

	enum AVPixelFormat src_pix_fmt=AV_PIX_FMT_YUV420P;
	RTMP_CreatePublish(url,outSize,1,RTMP_LOGINFO);
	printf("connected \n");
	RTMP_InitVideoParams(width,height,fps,rate,src_pix_fmt,false);
	printf("inited \n");
	runflag=1;
	unsigned int tick = 0;
	unsigned int tick_gap = 1000/fps;
	uint32_t now=0,last_update=0;
	int index=0;
	while(runflag){
		if(index!=0){
			RTMP_SendScreenCapture((char*)buffer,height,tick);
			printf("send frame index -- %d\n",index);
		}
		last_update=RTMP_GetTime();
		switch(src_pix_fmt){
			case AV_PIX_FMT_YUV420P:
				if (fread(buffer, 1, baseFrameSize*3/2, fp) != baseFrameSize*3/2){
					// Loop
					fseek(fp, 0, SEEK_SET);
					fread(buffer, 1, baseFrameSize*3/2, fp);
					//fclose(fp);
					//break;
				}
				printf("read file \n");
				break;
			case AV_PIX_FMT_YUV422P:
				if (fread(buffer, 1, baseFrameSize*2, fp) != baseFrameSize*2){
					// Loop
					fseek(fp, 0, SEEK_SET);
					fread(buffer, 1, baseFrameSize*2, fp);
					//fclose(fp);
					//break;
				}
					break;
			case AV_PIX_FMT_RGB24:
				if (fread(buffer, 1, baseFrameSize*3, fp) != baseFrameSize*3){
					// Loop
					fseek(fp, 0, SEEK_SET);
					fread(buffer, 1, baseFrameSize*3, fp);
					//fclose(fp);
					//break;
				}
					break;
			default:
				printf("Not supports this format \n");
				break;
			}
		tick +=tick_gap;
		now=RTMP_GetTime();
		usleep((tick_gap-now+last_update)*1000);
		index++;
	}


	RTMP_DeletePublish();
	fclose(fp);
}
