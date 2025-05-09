#ifndef SPLITTER_H
#define SPLITTER_H

namespace Cnvt
{
	typedef unsigned char uint8_t;
	typedef unsigned int  UINT;
	typedef unsigned short USHORT;
	typedef unsigned long  ULONG;
	USHORT GetOneNalu(unsigned char *pBufIn, USHORT nInSize, unsigned char *pNalu, USHORT &nNaluSize);
	int IsVideojjSEI(unsigned char *pNalu, int nNaluSize);

	int GetOneAACFrame(unsigned char *pBufIn, int nInSize, unsigned char *pAACFrame, int &nAACFrameSize);

	bool startCode3(unsigned char* data);
	bool startCode4(unsigned char* data);
	USHORT  getApartLen(uint8_t* data);
	USHORT getNextNalu(uint8_t* inputBuff, USHORT inputLen);
}


#endif // SPLITTER_H
