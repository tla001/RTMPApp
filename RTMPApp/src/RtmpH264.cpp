// RtmpH264.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "RtmpH264.h"


char* audioConfig = NULL;
long audioConfigLen = 0;

void stream_stop(RTMPMOD_SPublishObj* psObj);


x264_t * h = NULL;//��������
x264_picture_t m_picInput;//����ͼ��YUV
x264_picture_t m_picOutput;//���ͼ��RAW
x264_param_t param;//��������


unsigned int  nSendCount = 0;
x264_nal_t* nal_t = NULL;
int i_nal = 0;
unsigned char *pps = 0;
int pps_len;
unsigned char * sps = 0;
int sps_len;

RtmpH264* pRtmpH264 = NULL;


// Class constructor
RtmpH264::RtmpH264() : m_isCreatePublish(false),m_SwsContext(NULL)
{
	rtmp_PublishObj.hEncoder = 0;
	rtmp_PublishObj.nSampleRate = 0;
	rtmp_PublishObj.nChannels = 0;
	rtmp_PublishObj.nTimeDelta = 0;
	rtmp_PublishObj.szPcmAudio = 0;
	rtmp_PublishObj.nPcmAudioSize = 0;
	rtmp_PublishObj.nPcmAudioLen = 0;
	rtmp_PublishObj.szAacAudio = 0;
	rtmp_PublishObj.nAacAudioSize = 0;

}

//������������
int RtmpH264::CreatePublish(char* url, int outChunkSize, int isOpenPrintLog, int logType)
{
	int rResult = 0;
	rResult = RTMP264_Connect(url, &rtmp_PublishObj.rtmp, isOpenPrintLog, logType);

	if (rResult == 1)
	{
		m_isCreatePublish = true;
		m_startTime = RTMP_GetTime();

		//�޸ķ��ͷְ��Ĵ�С  Ĭ��128�ֽ�
		RTMPPacket pack;
		RTMPPacket_Alloc(&pack, 4);
		pack.m_packetType = RTMP_PACKET_TYPE_CHUNK_SIZE;
		pack.m_nChannel = 0x02;
		pack.m_headerType = RTMP_PACKET_SIZE_LARGE;
		pack.m_nTimeStamp = 0;
		pack.m_nInfoField2 = 0;
		pack.m_nBodySize = 4;
		pack.m_body[3] = outChunkSize & 0xff; //���ֽ���
		pack.m_body[2] = outChunkSize >> 8;
		pack.m_body[1] = outChunkSize >> 16;
		pack.m_body[0] = outChunkSize >> 24;
		rtmp_PublishObj.rtmp->m_outChunkSize = outChunkSize;
		RTMP_SendPacket(rtmp_PublishObj.rtmp,&pack,1);
		RTMPPacket_Free(&pack);
	}

	return rResult;
}

//�Ͽ���������
void RtmpH264::DeletePublish()
{
	rtmp_PublishObj.hEncoder = 0;
	rtmp_PublishObj.nSampleRate = 0;
	rtmp_PublishObj.nChannels = 0;
	rtmp_PublishObj.nTimeDelta = 0;
	rtmp_PublishObj.szPcmAudio = 0;
	rtmp_PublishObj.nPcmAudioSize = 0;
	rtmp_PublishObj.nPcmAudioLen = 0;
	rtmp_PublishObj.szAacAudio = 0;
	rtmp_PublishObj.nAacAudioSize = 0;

	if (m_isCreatePublish)
	{
		RTMP264_Close();
	}


	m_isCreatePublish = false;
}

/*��ʼ����Ƶ������
<param name = "width">��Ƶ����< / param>
*<param name = "height">��Ƶ�߶�< / param>
*<param name = "fps">֡��< / param>
*<param name = "bitrate">������< / param>
*<param name = "bConstantsBitrate"> �Ƿ�㶨���� < / param>
*/
int RtmpH264::InitVideoParams(unsigned long width, unsigned long height, unsigned long fps, unsigned long bitrate, enum AVPixelFormat src_pix_fmt,bool bConstantsBitrate = false)
{
	m_width = width;//��������ʵ�������
	m_height = height;//��
	m_frameRate = fps;
	m_bitRate = bitrate;
	m_srcPicFmt=src_pix_fmt;
	m_baseFrameSize=m_width*m_height;

	x264_param_default(&param);//����Ĭ�ϲ��������common/common.c


	//* ʹ��Ĭ�ϲ�������������Ϊ��ʵʱ���紫�䣬����ʹ����zerolatency��ѡ�ʹ�����ѡ��֮��Ͳ�����delayed_frames�����ʹ�õĲ��������Ļ�������Ҫ�ڱ������֮��õ�����ı���֡
	x264_param_default_preset(&param, "veryfast", "zerolatency");

	//* cpuFlags
	param.i_threads = X264_SYNC_LOOKAHEAD_AUTO; /* ȡ�ջ���������ʹ�ò������ı�֤ */
	param.i_sync_lookahead = X264_SYNC_LOOKAHEAD_AUTO;

	//* ��Ƶѡ��
	param.i_width = m_width;
	param.i_height = m_height;
	param.i_frame_total = 0; //* ������֡��.��֪����0.
	param.i_keyint_min = 0;//�ؼ�֡��С���
	param.i_keyint_max = (int)fps*2;//�ؼ�֡�����
	param.b_annexb = 1;//1ǰ��Ϊ0x00000001,0Ϊnal����
	param.b_repeat_headers = 0;//�ؼ�֡ǰ���Ƿ��sps��pps֡��0 �� 1����


	param.i_csp = X264_CSP_I420;


	//* B֡����
	param.i_bframe = 0;	//B֡
	param.b_open_gop = 0;
	param.i_bframe_pyramid = 0;
	param.i_bframe_adaptive = X264_B_ADAPT_FAST;

	//* ���ʿ��Ʋ���
	param.rc.i_lookahead = 0;
	param.rc.i_bitrate = (int)m_bitRate; //* ����(������,��λKbps)

	if(!bConstantsBitrate)
	{
		param.rc.i_rc_method = X264_RC_ABR;//����i_rc_method��ʾ���ʿ��ƣ�CQP(�㶨����)��CRF(�㶨����)��ABR(ƽ������)
		param.rc.i_vbv_max_bitrate = (int)m_bitRate*1; // ƽ������ģʽ�£����˲ʱ���ʣ�Ĭ��0(��-B������ͬ)
	}
	else
	{
		param.rc.b_filler = 1;

		param.rc.i_rc_method = X264_RC_ABR;//����i_rc_method��ʾ���ʿ��ƣ�CQP(�㶨����)��CRF(�㶨����)��ABR(ƽ������)
		param.rc.i_vbv_max_bitrate = m_bitRate; // ƽ������ģʽ�£����˲ʱ���ʣ�Ĭ��0(��-B������ͬ)
		param.rc.i_vbv_buffer_size  = m_bitRate; //vbv-bufsize
	}

	//* muxing parameters
	param.i_fps_den = 1; // ֡�ʷ�ĸ
	param.i_fps_num = fps;// ֡�ʷ���
	param.i_timebase_num = 1;
	param.i_timebase_den = 1000;


	h = x264_encoder_open(&param);//���ݲ�����ʼ��X264����

	x264_picture_init(&m_picOutput);//��ʼ��ͼƬ��Ϣ
	x264_picture_alloc(&m_picInput, X264_CSP_I420, m_width, m_height);//ͼƬ��I420��ʽ����ռ䣬���Ҫx264_picture_clean
	m_picInput.i_pts = 0;

	i_nal = 0;
	x264_encoder_headers(h, &nal_t, &i_nal);
	if (i_nal > 0)
	{
		for (int i = 0; i < i_nal; i++)
		{
			//��ȡSPS���ݣ�PPS����
			if (nal_t[i].i_type == NAL_SPS)
			{
				sps = new unsigned char[nal_t[i].i_payload - 4];
				sps_len = nal_t[i].i_payload - 4;
				memcpy(sps, nal_t[i].p_payload + 4, nal_t[i].i_payload - 4);
			}
			else if (nal_t[i].i_type == NAL_PPS)
			{
				pps = new unsigned char[nal_t[i].i_payload - 4];;
				pps_len = nal_t[i].i_payload - 4;
				memcpy(pps, nal_t[i].p_payload + 4, nal_t[i].i_payload - 4);
			}
		}

		InitSpsPps(pps, pps_len, sps, sps_len, m_width, m_height, m_frameRate);

		m_SwsContext= sws_getContext(m_width, m_height, /*AV_PIX_FMT_BGR24*/m_srcPicFmt, m_width, m_height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

		int ret;
		/* allocate source and destination image buffers */
		if ((ret = av_image_alloc(src_data, src_linesize,
				m_width, m_height, m_srcPicFmt, 8)) < 0) {
			fprintf(stderr, "Could not allocate source image\n");
			exit(-1);
		}
		return 1;
	}



	return 0;
}

//�ͷű������ڴ�
void RtmpH264::FreeEncodeParams()
{
	if (pps)
	{
		delete[] pps;
		pps = NULL;
	}

	if (sps)
	{
		delete[] sps;
		sps = NULL;
	}


	if(h)
	{
		x264_encoder_close(h);
		h = NULL;
	}

	if(m_SwsContext)
	{
		av_freep(&src_data[0]);
		sws_freeContext(m_SwsContext);
		m_SwsContext = NULL;
	}

	stream_stop(&rtmp_PublishObj);

	//DeleteCriticalSection(&m_Cs);

}

/**
* ͼƬ���뷢��
*
* @�ɹ��򷵻� 1 , ʧ���򷵻�0
*/
int RtmpH264::SendScreenCapture(BYTE * frame,  unsigned long StrideHeight, unsigned long timespan)
{

	//int nDataLen = Stride * StrideHeight;

	/*uint8_t * rgb_buff = new uint8_t[nDataLen];
	memcpy(rgb_buff, frame, nDataLen);*/

	//�����λͼת���ǵ�����
//	uint8_t *rgb_src[3] = { frame, NULL, NULL };
//	int rgb_stride[3]={Stride, 0, 0};
//	sws_scale(m_SwsContext, rgb_src, rgb_stride, 0, m_height, m_picInput.img.plane, m_picInput.img.i_stride);

	//delete[] rgb_buff;
	switch(m_srcPicFmt){
	case AV_PIX_FMT_YUV420P:
		memcpy(src_data[0],frame,m_baseFrameSize);
		memcpy(src_data[1],frame+m_baseFrameSize,m_baseFrameSize/4);
		memcpy(src_data[2],frame+m_baseFrameSize*5/4,m_baseFrameSize/4);
		break;
	case AV_PIX_FMT_YUV422P:
			memcpy(src_data[0],frame,m_baseFrameSize*2);
			break;
	case AV_PIX_FMT_RGB24:
			memcpy(src_data[0],frame,m_baseFrameSize*3);
			break;
	default:
		printf("Not supports this format \n");
		break;
	}
	sws_scale(m_SwsContext,src_data,src_linesize,0,m_height, m_picInput.img.plane, m_picInput.img.i_stride);

	i_nal = 0;
	x264_encoder_encode(h, &nal_t, &i_nal, &m_picInput, &m_picOutput);
	m_picInput.i_pts++;//�����Ļ������ x264 [warning]: non-strictly-monotonic PTS


	int rResult = 0;
	for (int i = 0; i < i_nal; i++)
	{
		int bKeyFrame = 0;
		//��ȡ֡����
		if (nal_t[i].i_type == NAL_SLICE || nal_t[i].i_type == NAL_SLICE_IDR)
		{
			if (nal_t[i].i_type == NAL_SLICE_IDR)
				bKeyFrame = 1;

			//EnterCriticalSection(&m_Cs);
			rResult = SendH264Packet(nal_t[i].p_payload + 4, nal_t[i].i_payload - 4, bKeyFrame, timespan);
			//LeaveCriticalSection(&m_Cs);
		}
	}

	return rResult;
}

#define EB_MAX(a,b)               (((a) > (b)) ? (a) : (b))
#define EB_MIN(a,b)               (((a) < (b)) ? (a) : (b))

#define PCM_BITSIZE               2
//ֹͣ��Ƶ����
void stream_stop(RTMPMOD_SPublishObj* psObj)
{
	int						nRet;
	if (psObj->hEncoder)
	{
		while (1)
		{
			nRet = faacEncEncode(psObj->hEncoder, 0, 0, (unsigned char*)(psObj->szAacAudio + RTMP_MAX_HEADER_SIZE + 2), psObj->nAacAudioSize - RTMP_MAX_HEADER_SIZE - 2);
			if (0 == nRet) break;
		}
		nRet = faacEncClose(psObj->hEncoder);
	}
	if (psObj->szPcmAudio)
	{
		free(psObj->szPcmAudio);
	}
	if (psObj->szAacAudio)
	{
		free(psObj->szAacAudio);
	}
	psObj->hEncoder = 0;
	psObj->nSampleRate = 0;
	psObj->nChannels = 0;
	psObj->nTimeDelta = 0;
	psObj->szPcmAudio = 0;
	psObj->nPcmAudioSize = 0;
	psObj->nPcmAudioLen = 0;
	psObj->szAacAudio = 0;
	psObj->nAacAudioSize = 0;
}

//��ʼ����Ƶ������
bool stream_init(RTMPMOD_SPublishObj* psObj, unsigned long nSampleRate, unsigned long nChannels)
{
	faacEncConfigurationPtr pConfiguration;
	unsigned long			nInputSamples;
	unsigned long			nMaxOutputBytes;
	int						nRet;
	char*					szPcmAudio;
	unsigned long			nPcmAudioSize;
	char*					szAacAudio;
	unsigned long			nAacAudioSize;
	unsigned char *			buf;
	unsigned long			len;

	// skip
	if ((psObj->hEncoder) && (nSampleRate == psObj->nSampleRate) && (nChannels == psObj->nChannels))
	{
		return true;
	}

	// close
	stream_stop(psObj);

	// open FAAC engine
	faacEncHandle hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
	if (hEncoder == NULL)
	{
		//assert(0);
		return false;
	}

	// set encoding configuration
	pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
	pConfiguration->aacObjectType = LOW;
	pConfiguration->bitRate = 32000;
	pConfiguration->mpegVersion = MPEG4;
	pConfiguration->allowMidside = 0;
	pConfiguration->useTns = 0;
	pConfiguration->useLfe = 0;
	pConfiguration->bandWidth = 0;
	pConfiguration->quantqual = 100;
	pConfiguration->outputFormat = 0;
	pConfiguration->inputFormat = (PCM_BITSIZE == 2) ? FAAC_INPUT_16BIT : FAAC_INPUT_32BIT;
	pConfiguration->shortctl = SHORTCTL_NORMAL;
	nRet = faacEncSetConfiguration(hEncoder, pConfiguration);
	if (!nRet)
	{
		//assert(0);
		nRet = faacEncClose(hEncoder);
		return false;
	}

	// buffer
	nPcmAudioSize = nInputSamples * PCM_BITSIZE * nChannels;
	szPcmAudio = (char*)malloc(nPcmAudioSize);
	nAacAudioSize = nMaxOutputBytes + RTMP_MAX_HEADER_SIZE + 2;
	szAacAudio = (char*)malloc(nAacAudioSize);
	if ((!szPcmAudio) || (!szAacAudio))
	{
		nRet = faacEncClose(hEncoder);
		if (szPcmAudio)
		{
			free(szPcmAudio);
		}
		if (szAacAudio)
		{
			free(szAacAudio);
		}
		//assert(0);
		return false;
	}

	// success
	psObj->hEncoder = hEncoder;
	psObj->nSampleRate = nSampleRate;
	psObj->nChannels = nChannels;
	psObj->nTimeDelta = (nInputSamples * 1000 / nSampleRate);
	psObj->szPcmAudio = szPcmAudio;
	psObj->nPcmAudioSize = nPcmAudioSize;
	psObj->nPcmAudioLen = 0;
	psObj->szAacAudio = szAacAudio;
	psObj->nAacAudioSize = nAacAudioSize;

	// send aac header
	faacEncGetDecoderSpecificInfo(hEncoder, &buf, &len);
	memcpy(psObj->szAacAudio + RTMP_MAX_HEADER_SIZE + 2, buf, len);
	len += 2;
	psObj->szAacAudio[RTMP_MAX_HEADER_SIZE + 0] = (char)0xAF;
	psObj->szAacAudio[RTMP_MAX_HEADER_SIZE + 1] = (char)0x00;
	psObj->packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	psObj->packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
	psObj->packet.m_hasAbsTimestamp = 0;
	psObj->packet.m_nChannel = 0x04;
	psObj->packet.m_nTimeStamp = psObj->nTimeStamp;
	psObj->packet.m_nInfoField2 = psObj->rtmp->m_stream_id;
	psObj->packet.m_nBodySize = len;
	psObj->packet.m_body = psObj->szAacAudio + RTMP_MAX_HEADER_SIZE;
	psObj->packet.m_nBytesRead = 0;

	// send packet
	//assert(psObj->rtmp);

	//EnterCriticalSection(&m_Cs);

	RTMP_SendPacket(psObj->rtmp, &psObj->packet, true);

	//LeaveCriticalSection(&m_Cs);

	// success
	return true;
}

//���������Ƶ����
long RTMPMOD_PublishSendAudio(RTMPMOD_SPublishObj* hObj, char* szBuf, unsigned long nBufLen, unsigned long nSampleRate, unsigned long nChannels, unsigned long timespan)
{


	RTMPMOD_SPublishObj*	psObj = (RTMPMOD_SPublishObj*)hObj;
	unsigned long			nDone = 0;
	unsigned long			nCopy;
	int						nRet;

	// check
	if ((!psObj) || (!szBuf) || (!nBufLen) || (!nSampleRate) || ((1 != nChannels) && (2 != nChannels)))
	{
		return 0;
	}

	// init
	if (!psObj->rtmp)
	{
		return 0;
	}
	if (!stream_init(psObj, nSampleRate, nChannels))
	{
		return 0;
	}

	// encode and send
	while (nDone < nBufLen)
	{
		// copy
		nCopy = EB_MIN((nBufLen - nDone), (psObj->nPcmAudioSize - psObj->nPcmAudioLen));
		memcpy(psObj->szPcmAudio + psObj->nPcmAudioLen, szBuf + nDone, nCopy);
		nDone += nCopy;
		psObj->nPcmAudioLen += nCopy;



		// ready
		if (psObj->nPcmAudioLen == psObj->nPcmAudioSize)
		{
			// encode
			nRet = faacEncEncode(psObj->hEncoder, (int*)psObj->szPcmAudio, (psObj->nPcmAudioSize / PCM_BITSIZE) / psObj->nChannels,
				(unsigned char*)(psObj->szAacAudio + RTMP_MAX_HEADER_SIZE + 2), psObj->nAacAudioSize - RTMP_MAX_HEADER_SIZE - 2);
			psObj->nPcmAudioLen = 0;
			if (nRet <= 0)
			{
				continue;
			}
			psObj->nTimeStamp = timespan;
			//psObj->nTimeStamp += psObj->nTimeDelta;

			// packet
			psObj->szAacAudio[RTMP_MAX_HEADER_SIZE + 0] = (char)0xAF;
			psObj->szAacAudio[RTMP_MAX_HEADER_SIZE + 1] = (char)0x01;
			psObj->packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
			psObj->packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;
			psObj->packet.m_hasAbsTimestamp = 0;
			psObj->packet.m_nChannel = 0x04;
			psObj->packet.m_nTimeStamp = psObj->nTimeStamp;
			psObj->packet.m_nInfoField2 = psObj->rtmp->m_stream_id;
			psObj->packet.m_nBodySize = nRet + 2;
			psObj->packet.m_body = psObj->szAacAudio + RTMP_MAX_HEADER_SIZE;
			psObj->packet.m_nBytesRead = 0;


			//EnterCriticalSection(&m_Cs);

			// send packet
			if (!RTMP_SendPacket(psObj->rtmp, &psObj->packet, true))
			{
				return 0;
			}

			//LeaveCriticalSection(&m_Cs);
		}

	}


	// success
	return 1;
}


//��������������
long RTMP_CreatePublish(char* url, unsigned long outChunkSize, int isOpenPrintLog, int logType)
{
	int nResult = -1;
	if (pRtmpH264)
	{
		pRtmpH264->FreeEncodeParams();
		pRtmpH264->DeletePublish();

		delete pRtmpH264;
	}

	pRtmpH264 = new RtmpH264();
	nResult = pRtmpH264->CreatePublish(url, (int)outChunkSize, isOpenPrintLog, logType);
	if (nResult != 1)
		pRtmpH264->m_isCreatePublish = false;

	return nResult;
}

//�Ͽ�������
void RTMP_DeletePublish()
{
	if (pRtmpH264)
	{
		pRtmpH264->FreeEncodeParams();
		pRtmpH264->DeletePublish();
		delete pRtmpH264;
	}
	pRtmpH264 = NULL;
}

//��ʼ��������
long RTMP_InitVideoParams(unsigned long width, unsigned long height, unsigned long fps, unsigned long bitrate, enum AVPixelFormat src_pix_fmt,bool bConstantsBitrate)
{
	int nResult = -1;
	if (!pRtmpH264)
	{
		return -1;
	}

		nResult = pRtmpH264->InitVideoParams(width, height, fps, bitrate,src_pix_fmt, bConstantsBitrate);
	if( nResult<0)
	{
		nResult = -1;
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
		throw(-1);
	}

	if (nResult != 1)
	{
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
	}

	return nResult;
}

//���ͽ�ͼ  �Խ�ͼ����x264����
long RTMP_SendScreenCapture(char * frame,  unsigned long Height, unsigned long timespan)
{
	int nResult = -1;
	if (!pRtmpH264)
	{
		return -1;
	}


	if (!pRtmpH264->m_isCreatePublish)
		return -1;
	nResult = pRtmpH264->SendScreenCapture((BYTE *)frame, Height, timespan);
	if(nResult<0)
	{
		nResult = -1;
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
		throw(-1);
	}

	if (nResult != 1)
	{
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
	}

	return nResult;
}

//������Ƶ����  ԭʼ����ΪPCM
long RTMP_SendAudioFrame(char* szBuf, unsigned long nBufLen, unsigned long nSampleRate, unsigned long nChannels, unsigned long timespan)
{
	int nResult = -1;
	if (!pRtmpH264)
	{
		return -1;
	}

	if (!pRtmpH264->m_isCreatePublish)
		return -1;
	nResult = RTMPMOD_PublishSendAudio(&pRtmpH264->rtmp_PublishObj, szBuf, nBufLen, nSampleRate, nChannels, timespan);
	if(nResult<0)
	{
		nResult = -1;
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
		throw(-1);
	}
	if (nResult != 1)
	{
		pRtmpH264->m_isCreatePublish = false;
		RTMP_DeletePublish();
	}

	return nResult;
}