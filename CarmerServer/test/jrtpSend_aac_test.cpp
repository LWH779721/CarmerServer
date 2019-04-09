#include "rtpsession.h"
#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtptimeutilities.h"
#include "rtppacket.h"

#include <faac.h>

#include <stdlib.h>
#include <iostream>

#define SSRC           100
typedef unsigned long   ULONG;
typedef unsigned int    UINT;
typedef unsigned char   BYTE;

using namespace jrtplib;

int main(int argc, char** argv)
{
	RTPSession session;
	RTPSessionParams sessionparams;
	RTPUDPv4TransmissionParams transparams;
	
	sessionparams.SetOwnTimestampUnit(1.0/43.0);
	transparams.SetPortbase(8000);

	int status = session.Create(sessionparams,&transparams);
	if (status < 0)
	{
		std::cerr << RTPGetErrorString(status) << std::endl;
		exit(-1);
	}

	uint8_t localip[]={ 192,168, 70, 45};
	RTPIPv4Address addr(localip,9000);

	status = session.AddDestination(addr);
	if (status < 0)
	{
		std::cerr << RTPGetErrorString(status) << std::endl;
		exit(-1);
	}

	session.SetDefaultPayloadType(97);
	session.SetDefaultMark(false);
	session.SetDefaultTimestampIncrement(23);

	RTPTime delay(0.023);
	RTPTime starttime = RTPTime::CurrentTime();

	char sendbuf[1500];
	char* nalu_payload;
	unsigned int timestamp_increse=0,ts_current=0;
	
    ULONG nSampleRate = 44100;  // 采样率
    UINT nChannels = 1;         // 声道数
    UINT nPCMBitSize = 16;      // 单样本位数
    ULONG nInputSamples = 0;
    ULONG nMaxOutputBytes = 0;
 
    int nRet;
    faacEncHandle hEncoder;
    faacEncConfigurationPtr pConfiguration;
 
    int nBytesRead;
    int nPCMBufferSize;
    BYTE* pbPCMBuffer;
    BYTE* pbAACBuffer;
 
    FILE* fpIn;
 
    fpIn = fopen(argv[1], "rb");
	
    hEncoder = faacEncOpen(nSampleRate, nChannels, &nInputSamples, &nMaxOutputBytes);
    if(hEncoder == NULL)
    {
        printf("[ERROR] Failed to call faacEncOpen()\n");
        return -1;
    }

    nPCMBufferSize = nInputSamples * nPCMBitSize / 8;
    pbPCMBuffer = new BYTE [nPCMBufferSize];
    pbAACBuffer = new BYTE [nMaxOutputBytes];
 
    // (2.1) Get current encoding configuration
    pConfiguration = faacEncGetCurrentConfiguration(hEncoder);
	
	pConfiguration->aacObjectType = LOW;    //LC编码
	pConfiguration->mpegVersion = MPEG4;  //
	pConfiguration->useTns   = 1 ;                   //时域噪音控制,大概就是消爆音
	pConfiguration->allowMidside = 0 ;            //
	pConfiguration->bitRate  = 0;
	pConfiguration->bandWidth  = 0 ;              //频宽
	pConfiguration->outputFormat = 1;  //输出是否包含ADTS头
	pConfiguration->inputFormat = FAAC_INPUT_16BIT;
	pConfiguration->quantqual = 100 ;

    // (2.2) Set encoding configuration
    nRet = faacEncSetConfiguration(hEncoder, pConfiguration);
	
	sendbuf[0] = 0x00;
	sendbuf[1] = 0x10;
	for(int i = 0; 1; i++)
    {
        // 读入的实际字节数，最大不会超过nPCMBufferSize，<br data-filtered="filtered">      
		nBytesRead = fread(pbPCMBuffer, 1, nPCMBufferSize, fpIn);
        // 输入样本数，用实际读入字节数计算，一般只有读到文件尾时才不是nPCMBufferSize/(nPCMBitSize/8);
        nInputSamples = nBytesRead / (nPCMBitSize / 8);
		printf("%d .... %d\n", nBytesRead, nInputSamples);
        // (3) Encode
        nRet = faacEncEncode(
            hEncoder, (int*) pbPCMBuffer, nInputSamples, pbAACBuffer, nMaxOutputBytes);
 
		if (nRet == 0)
		{
			continue;
		}
		
		sendbuf[2] = ((nRet - 7) & 0x1fe0) >> 5;
		sendbuf[3] = ((nRet - 7) & 0x1f) << 3;
		memcpy(sendbuf + 4, pbAACBuffer + 7, nRet - 7);
		
		status = session.SendPacket((void *)sendbuf, nRet - 3,97,true, 24);
		if (status < 0)
		{
			std::cerr << RTPGetErrorString(status) << std::endl;
			exit(-1);
		}

        if(nBytesRead <= 0)
        {
            //break;
			rewind(fpIn);
        }
		
		RTPTime::Wait(delay);
    }
	
	printf("over\n");
	delay = RTPTime(10.0);
	session.BYEDestroy(delay,"Time's up",9);
	
	nRet = faacEncClose(hEncoder);
 
    delete[] pbPCMBuffer;
    delete[] pbAACBuffer;
    fclose(fpIn);
}







