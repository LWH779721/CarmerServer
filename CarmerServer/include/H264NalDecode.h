#ifndef __H264NALDECODE_H__
#define __H264NALDECODE_H__

#ifdef __cplusplus
extern "C"{
#endif

#define MAX_RTP_PKT_LENGTH     1360

typedef struct {
	unsigned char TYPE:5;
    unsigned char NRI:2;
	unsigned char F:1;         
} NALU_HEADER; 

typedef struct {
    unsigned char TYPE:5;
	unsigned char NRI:2; 
	unsigned char F:1;          
} FU_INDICATOR; 

typedef struct {
    unsigned char TYPE:5;
	unsigned char R:1;
	unsigned char E:1;
	unsigned char S:1;    
} FU_HEADER; 

typedef struct {
	FILE *fp;
	int start_code;
	int end_code;
	
	struct {
		unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
		unsigned char buf[8000000];	 //! Nalu data		
		int forbidden_bit;            //! should be always FALSE
		int nal_reference_idc;        //! NALU_PRIORITY_xxxx
		int nal_unit_type;            //! NALU_TYPE_xxxx		
	} NALU;
} H264NalDecoder;

extern H264NalDecoder *H264NalDecode_init(const char *h264file);
extern int GetNextAnnexbNALU (H264NalDecoder *decoder);

#ifdef __cplusplus
}
#endif
#endif
