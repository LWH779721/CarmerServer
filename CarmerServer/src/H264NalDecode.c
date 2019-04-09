#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "H264NalDecode.h"

H264NalDecoder *H264NalDecode_init(const char *h264file)
{
	H264NalDecoder *decoder = NULL;

	if (NULL == (decoder = (H264NalDecoder *)malloc(sizeof(H264NalDecoder))))
	{
		return NULL;	
	}
	
	if (NULL == (decoder->fp = fopen(h264file, "rb")))
	{
		printf("open file error\n");
		free(decoder);
		return NULL;
	}
	
	decoder->start_code = 0;
	decoder->end_code = 0;
	decoder->NALU.len = 0;
	
	return decoder;
}

/*
* 找到 NALU 起始位置
* 以 0x000001 或 0x00000001 开始
*/
static int findStartCode(H264NalDecoder *decoder)
{
	int data, zero = 0;
	
	while(1)
	{
		data = fgetc(decoder->fp);	
		if (feof (decoder->fp))
		{
			return -1;
		}
		
		if (data == 0x00)
		{
			zero++;
		}
		else if (data == 0x01)
		{
			if (zero >= 2)
			{
				return 0;
			}
		}
		else
		{
			zero = 0;	
		}
	}
	
	return 0;
}

static int findStartCodeAfterEndCode(H264NalDecoder *decoder)
{
	int data, zero = 3;
	
	while(1)
	{
		data = fgetc(decoder->fp);	
		if (feof (decoder->fp))
		{
			return -1;
		}
		
		if (data == 0x00)
		{
			zero++;
		}
		else if (data == 0x01)
		{
			if (zero >= 2)
			{
				return 0;
			}
		}
		else
		{
			zero = 0;	
		}
	}
	
	return 0;
}

int GetNextAnnexbNALU (H264NalDecoder *decoder)
{
	int ret = -1;
	int data, zero = 0;
	
	if (decoder->start_code == 1)
	{
	}
	else if (decoder->end_code == 1)
	{
		ret = findStartCodeAfterEndCode(decoder);
		if (ret == -1)
		{
			return -1;
		}
	}
	else
	{
		ret = findStartCode(decoder);
		if (ret == -1)
		{
			return -1;
		}
	}
	
	decoder->NALU.len = 0;
	while (1)
	{
		data = fgetc (decoder->fp);
		if (feof (decoder->fp))
		{
			decoder->NALU.forbidden_bit = decoder->NALU.buf[0] & 0x80; //1 bit
			decoder->NALU.nal_reference_idc = decoder->NALU.buf[0] & 0x60; // 2 bit
			decoder->NALU.nal_unit_type = decoder->NALU.buf[0] & 0x1f;// 5 bit
			
			decoder->start_code = 0;
			decoder->end_code = 0;
			
			printf("NALU len: %d, nal_unit_type: %x\n", decoder->NALU.len, decoder->NALU.nal_unit_type);
			return decoder->NALU.len;	
		}
			
		decoder->NALU.buf[decoder->NALU.len++] = data;
		if (data == 0)
		{
			zero++;
			if (zero == 3)
			{
				decoder->end_code = 1;
				decoder->start_code = 0;
				decoder->NALU.len -= 3;
				break;
			}
		}
		else if (data == 1)
		{
			if (zero == 2)
			{
				decoder->end_code = 0;
				decoder->start_code = 1;
				decoder->NALU.len -= 3;
				break;	
			}
			else
			{
				zero = 0;	
			}
		}
		else
		{
			zero = 0;	
		}
	}

	decoder->NALU.forbidden_bit = decoder->NALU.buf[0] & 0x80; //1 bit
	decoder->NALU.nal_reference_idc = decoder->NALU.buf[0] & 0x60; // 2 bit
	decoder->NALU.nal_unit_type = decoder->NALU.buf[0] & 0x1f;// 5 bit
	
	printf("NALU len: %d, nal_unit_type: %x\n", decoder->NALU.len, decoder->NALU.nal_unit_type);
	
	return decoder->NALU.len;	
}