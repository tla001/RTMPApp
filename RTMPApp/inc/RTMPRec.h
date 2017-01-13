/*
 * RTMPRec.h
 *
 *  Created on: Jan 11, 2017
 *      Author: tla001
 */

#ifndef RTMPREC_H_
#define RTMPREC_H_

extern "C"{
#include <stdio.h>
#include <stdlib.h>
#include "printlog.h"
}
#include <iostream>
#include <string>

#include "ThreadBase.h"
#include "librtmp/rtmp.h"
#include "librtmp/log.h"


using namespace std;

class RTMPRec :public ThreadBase{
public:
	explicit RTMPRec(const string url,const string filename);
	virtual ~RTMPRec();
	int init();
	void run();
	void worker();
	void doSave();


private:
	string rtmpUrl;
	string outFile;
	int bufSize;
	char *buf;
	FILE *fp;
	long countSize;
	bool b_live_stream ;

	RTMP *rtmp;


public:
	static void test(){
		string url("rtmp://localhost/live/test1");
		string filename("rec.flv");
		RTMPRec *rec=new RTMPRec(url,filename);
		if(rec->init()<0){
			log_err("Error when init");
			exit(-1);
		}
		rec->doSave();

		rec->join();
	}
};

#endif /* RTMPREC_H_ */
