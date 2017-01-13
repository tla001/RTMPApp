/*
 * RTMPRec.cpp
 *
 *  Created on: Jan 11, 2017
 *      Author: tla001
 */

#include "RTMPRec.h"
#include "sockInit.h"

RTMPRec::RTMPRec(const string url,const string filename) {
	// TODO Auto-generated constructor stub
	rtmpUrl=url;
	outFile=filename;
	bufSize=1024*1024*10;
	buf=new char[bufSize];
	countSize=0;
	b_live_stream=true;
	rtmp=RTMP_Alloc();
}

RTMPRec::~RTMPRec() {
	// TODO Auto-generated destructor stub
	 if (fp != NULL) {
	        fclose(fp);
	        fp = NULL;
	    }

	if (buf != NULL) {
		delete[] buf;
		buf = NULL;
	}
	CleanupSockets();
	if (rtmp != NULL) {
		RTMP_Close(rtmp);
		RTMP_Free(rtmp);
		rtmp = NULL;
	}
}

int RTMPRec::init(){

	fp=fopen(outFile.c_str(),"wb");
	if(NULL==fp){
		RTMP_LogPrintf("Open File Error.\n");
		return -1;
	}
	InitSockets();
	RTMP_Init(rtmp);
	//set connection timeout,default 30s
	rtmp->Link.timeout=10;
	if (!RTMP_SetupURL(rtmp,const_cast<char*>(rtmpUrl.c_str()))) {
	        RTMP_Log(RTMP_LOGERROR, "SetupURL Err\n");
	        RTMP_Free(rtmp);
	        return -1;
	    }
	if (b_live_stream) {
		rtmp->Link.lFlags |= RTMP_LF_LIVE;
	}

	//1hour
	RTMP_SetBufferMS(rtmp, 3600 * 1000);

	if (!RTMP_Connect(rtmp, NULL)) {
		RTMP_Log(RTMP_LOGERROR, "Connect Err\n");
		RTMP_Free(rtmp);
		return -1;
	}

	if (!RTMP_ConnectStream(rtmp, 0)) {
		RTMP_Log(RTMP_LOGERROR, "ConnectStream Err\n");
		RTMP_Free(rtmp);
		RTMP_Close(rtmp);
		return -1;
	}
}
void RTMPRec::run(){
	worker();
}
void RTMPRec::worker(){
	int nread;
	while ((nread = RTMP_Read(rtmp, buf, bufSize)) != 0) {
		fwrite(buf, 1, (size_t)nread, fp);
		memset(buf,0,bufSize);
		countSize += nread;
		RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n", nread, countSize * 1.0 / 1024);
	}
}
void RTMPRec::doSave(){
	this->start();
}
