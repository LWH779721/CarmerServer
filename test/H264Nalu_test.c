#include <stdio.h>

#include "H264NalDecode.h"

int main(int argc, char **args)
{
	H264NalDecoder *decoder = NULL;
	
	decoder = H264NalDecode_init(args[1]);
	if (NULL == decoder)
	{
		printf("failed when init decoder\n");
		return -1;
	}
	
	while (GetNextAnnexbNALU(decoder) > 0)
	{
	}
	
	return 0;
}
