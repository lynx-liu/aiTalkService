#include <string.h>
#include <memory.h>
#include "splitter.h"

namespace Cnvt
{
	bool startCode3(uint8_t* data)
	{
		if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x01){
			return true;
		}
		return false;
	}
	bool startCode4(uint8_t* data)
	{
		if(data[0] == 0x00 && data[1] == 0x00 && data[2] == 0x00 && data[3] == 0x01){
			return true;
		}
		return false;
	}

	USHORT getApartLen(uint8_t* data)
	{
		USHORT apartLen2 = 0;
		if(data[2] == 0x01) apartLen2 = 3;
    	else if(data[3] == 0x01) apartLen2 = 4;

		return apartLen2;
	}

	USHORT getNextNalu(uint8_t* inputBuff, USHORT inputLen)
	{	
		USHORT index = 0;
		USHORT apartLen = 0;
		if(startCode4(inputBuff) || startCode3(inputBuff)){
			apartLen = getApartLen(inputBuff);
			if (0 == apartLen) return 0;
			
			for(index = apartLen; index<= inputLen; index++){
				if((index == inputLen) || startCode4(inputBuff+index) || startCode3(inputBuff+index)){
					return index;
				}
			}
		}
		return 0;
	}




	USHORT GetOneNalu(unsigned char *pBufIn, USHORT nInSize, unsigned char *pNalu, USHORT& nNaluSize)
	{
		unsigned char *p = pBufIn;
		USHORT nStartPos = 0, nEndPos = 0;
		while (1)
		{
			if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
			{
				nStartPos = p - pBufIn;
				break;
			}
			p++;
			if (p - pBufIn >= nInSize - 4)
				return 0;
		}
		p++;
		while (1)
		{
			if (p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01)
			{
				nEndPos = p - pBufIn;
				break;
			}
			p++;
			if (p - pBufIn >= nInSize - 4)
			{
				nEndPos = nInSize;
				break;
			}
		}
		nNaluSize = nEndPos - nStartPos;
		memcpy(pNalu, pBufIn + nStartPos, nNaluSize);

		return nNaluSize;
	}

	int IsVideojjSEI(unsigned char *pNalu, int nNaluSize)
	{
		unsigned char *p = pNalu;

		if (p[3] != 1 || p[4] != 6 || p[5] != 5)
			return 0;
		p += 6;
		while (*p++==0xff) ;
		const char *szVideojjUUID = "VideojjLeonUUID";
		char *pp = (char *)p;
		for (int i = 0; i < strlen(szVideojjUUID); i++)
		{
			if (pp[i] != szVideojjUUID[i])
				return 0;
		}

		return 1;
	}

	int GetOneAACFrame(unsigned char *pBufIn, int nInSize, unsigned char *pAACFrame, int &nAACFrameSize)
	{
		unsigned char *p = pBufIn;

		if (nInSize <= 7)
			return 0;

		int nFrameSize = ((p[3] & 0x3) << 11) + (p[4] << 3) + (p[5] >> 5);
		if (nInSize < nFrameSize)
			return 0;

		nAACFrameSize = nFrameSize;
		memcpy(pAACFrame, pBufIn, nFrameSize);

		return 1;
	}
}
