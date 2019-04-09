#include "rtpsession.h"
#include "rtpsessionparams.h"
#include "rtpudpv4transmitter.h"
#include "rtpipv4address.h"
#include "rtptimeutilities.h"
#include "rtppacket.h"

#include <stdlib.h>
#include <iostream>

#include "H264NalDecode.h"
#define SSRC           100

using namespace jrtplib;

int main(int argc, char** argv)
{
	H264NalDecoder *decoder = NULL;
	RTPSession session;
	RTPSessionParams sessionparams;
	
	decoder = H264NalDecode_init(argv[1]);
	if (NULL == decoder)
	{
		printf("failed when init decoder\n");
		return -1;
	}
	
	sessionparams.SetOwnTimestampUnit(1.0/90000.0);

	RTPUDPv4TransmissionParams transparams;
	transparams.SetPortbase(8000);

	int status = session.Create(sessionparams,&transparams);
	if (status < 0)
	{
		std::cerr << RTPGetErrorString(status) << std::endl;
		exit(-1);
	}

	uint8_t localip[]={ 192,168, 70, 45};
	RTPIPv4Address addr(localip,9400);

	status = session.AddDestination(addr);
	if (status < 0)
	{
		std::cerr << RTPGetErrorString(status) << std::endl;
		exit(-1);
	}

	session.SetDefaultPayloadType(96);
	session.SetDefaultMark(false);
	session.SetDefaultTimestampIncrement(90000.0 /25.0);

	RTPTime delay(0.040);
	RTPTime starttime = RTPTime::CurrentTime();

	NALU_HEADER		*nalu_hdr;
	FU_INDICATOR	*fu_ind;
	FU_HEADER		*fu_hdr;
	char sendbuf[1500];
	char* nalu_payload;
	unsigned int timestamp_increse=0,ts_current=0;

	bool start=false;
	while (GetNextAnnexbNALU(decoder) > 0)
	{
		if(!start)
		{
			if(decoder->NALU.nal_unit_type==5||decoder->NALU.nal_unit_type==6||
					decoder->NALU.nal_unit_type==7||decoder->NALU.nal_unit_type==7)
			{
				printf("begin\n");
				start=true;
			}
		}

		//	当一个NALU小于MAX_RTP_PKT_LENGTH字节的时候，采用一个单RTP包发送
			if(decoder->NALU.len<=MAX_RTP_PKT_LENGTH)
			{
			#if 0	
				//设置NALU HEADER,并将这个HEADER填入sendbuf[12]
				nalu_hdr =(NALU_HEADER*)&sendbuf[0]; //将sendbuf[12]的地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入sendbuf中；
				nalu_hdr->F=decoder->NALU.forbidden_bit;
				nalu_hdr->NRI=decoder->NALU.nal_reference_idc>>5;//有效数据在n->nal_reference_idc的第6，7位，需要右移5位才能将其值赋给nalu_hdr->NRI。
				nalu_hdr->TYPE=decoder->NALU.nal_unit_type;

				nalu_payload=&sendbuf[1];//同理将sendbuf[13]赋给nalu_payload
				memcpy(nalu_payload,decoder->NALU.buf+1,decoder->NALU.len-1);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。
				ts_current=ts_current+timestamp_increse;
			#else
				nalu_payload=&sendbuf[0];
				memcpy(nalu_payload,decoder->NALU.buf,decoder->NALU.len);//去掉nalu头的nalu剩余内容写入sendbuf[13]开始的字符串。
				ts_current=ts_current+timestamp_increse;
			#endif
		
				if(decoder->NALU.nal_unit_type==1 || decoder->NALU.nal_unit_type==5)
				{
					status = session.SendPacket((void *)sendbuf,decoder->NALU.len,96,true,3600);
				}
				else
				{
						status = session.SendPacket((void *)sendbuf,decoder->NALU.len,96,true,0);
						//如果是6,7类型的包，不应该延时；之前有停顿，原因这在这
						continue;
				}
				//发送RTP格式数据包并指定负载类型为96
				if (status < 0)
				{
					std::cerr << RTPGetErrorString(status) << std::endl;
					exit(-1);
				}

			}
			else if(decoder->NALU.len>MAX_RTP_PKT_LENGTH)
			{
				//得到该nalu需要用多少长度为MAX_RTP_PKT_LENGTH字节的RTP包来发送
				int k=0,l=0;
				k=decoder->NALU.len/MAX_RTP_PKT_LENGTH;//需要k个MAX_RTP_PKT_LENGTH字节的RTP包
				l=decoder->NALU.len%MAX_RTP_PKT_LENGTH;//最后一个RTP包的需要装载的字节数
				int t=0;//用于指示当前发送的是第几个分片RTP包
				ts_current=ts_current+timestamp_increse;
				while(t<=k)
				{

					if(!t)//发送一个需要分片的NALU的第一个分片，置FU HEADER的S位
					{
						//session.SetDefaultMark(false);
						//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
						fu_ind =(FU_INDICATOR*)&sendbuf[0]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
						fu_ind->F=decoder->NALU.forbidden_bit;
						fu_ind->NRI=decoder->NALU.nal_reference_idc>>5;
						fu_ind->TYPE=28;

						//设置FU HEADER,并将这个HEADER填入sendbuf[13]
						fu_hdr =(FU_HEADER*)&sendbuf[1];
						fu_hdr->E=0;
						fu_hdr->R=0;
						fu_hdr->S=1;
						fu_hdr->TYPE=decoder->NALU.nal_unit_type;


						nalu_payload=&sendbuf[2];//同理将sendbuf[14]赋给nalu_payload
						memcpy(nalu_payload,decoder->NALU.buf+1,MAX_RTP_PKT_LENGTH);//去掉NALU头

						status = session.SendPacket((void *)sendbuf,MAX_RTP_PKT_LENGTH+2,96,false,0);
						if (status < 0)
						{
							std::cerr << RTPGetErrorString(status) << std::endl;
							exit(-1);
						}
						t++;
					}
					//发送一个需要分片的NALU的非第一个分片，清零FU HEADER的S位，如果该分片是该NALU的最后一个分片，置FU HEADER的E位
					else if(k==t)//发送的是最后一个分片，注意最后一个分片的长度可能超过MAX_RTP_PKT_LENGTH字节（当l>1386时）。
					{
						//session.SetDefaultMark(true);
						//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
						fu_ind =(FU_INDICATOR*)&sendbuf[0]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
						fu_ind->F=decoder->NALU.forbidden_bit;
						fu_ind->NRI=decoder->NALU.nal_reference_idc>>5;
						fu_ind->TYPE=28;

						//设置FU HEADER,并将这个HEADER填入sendbuf[13]
						fu_hdr =(FU_HEADER*)&sendbuf[1];
						fu_hdr->R=0;
						fu_hdr->S=0;
						fu_hdr->TYPE=decoder->NALU.nal_unit_type;
						fu_hdr->E=1;
						nalu_payload=&sendbuf[2];//同理将sendbuf[14]赋给nalu_payload
						memcpy(nalu_payload,decoder->NALU.buf+t*MAX_RTP_PKT_LENGTH+1,l-1);//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串。

						status = session.SendPacket((void *)sendbuf,l+1,96,true,3600);
						if (status < 0)
						{
							std::cerr << RTPGetErrorString(status) << std::endl;
							exit(-1);
						}
						t++;
					}
					else if(t<k&&0!=t)
					{
						//session.SetDefaultMark(false);
						//设置FU INDICATOR,并将这个HEADER填入sendbuf[12]
						fu_ind =(FU_INDICATOR*)&sendbuf[0]; //将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
						fu_ind->F=decoder->NALU.forbidden_bit;
						fu_ind->NRI=decoder->NALU.nal_reference_idc>>5;
						fu_ind->TYPE=28;

						//设置FU HEADER,并将这个HEADER填入sendbuf[13]
						fu_hdr =(FU_HEADER*)&sendbuf[1];
						fu_hdr->R=0;
						fu_hdr->S=0;
						fu_hdr->E=0;
						fu_hdr->TYPE=decoder->NALU.nal_unit_type;

						nalu_payload=&sendbuf[2];//同理将sendbuf[14]的地址赋给nalu_payload
						memcpy(nalu_payload,decoder->NALU.buf+t*MAX_RTP_PKT_LENGTH+1,MAX_RTP_PKT_LENGTH);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串。

						status = session.SendPacket((void *)sendbuf,MAX_RTP_PKT_LENGTH+2,96,false,0);
						if (status < 0)
						{
							std::cerr << RTPGetErrorString(status) << std::endl;
							exit(-1);
						}
						t++;
					}
				}
			}

		RTPTime::Wait(delay);
	}
	printf("over\n");
	delay = RTPTime(10.0);
	session.BYEDestroy(delay,"Time's up",9);
}







