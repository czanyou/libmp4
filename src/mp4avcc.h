/***
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to ChengZhen(Anyou).  It is subject to the terms of a
 * License Agreement between Licensee and ChengZhen(Anyou).
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and
 * any derivative work shall retain this copyright notice.
 *
 * Copyright (c) 2014-2015 ChengZhen(Anyou). All Rights Reserved.
 *
 */
#ifndef _NS_VISION_MP4_AVCC_H
#define _NS_VISION_MP4_AVCC_H

#include "mp4common.h"
#include "mp4atom.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4AvcConfig class

/** 
 * 封装了对 AvcC atom 的操作.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4AvcConfig
{
public:
	Mp4AvcConfig(Mp4Atom* avcC);
	
// Operations -------------------------------------------------
public:
	BOOL AddPictureParameters (const BYTE* picture,  WORD pictureLen);
	BOOL AddSequenceParameters(const BYTE* sequence, WORD sequenceLen);

	BYTE* GetPictureParameters (int index, WORD& length);
	BYTE* GetSequenceParameters(int index, WORD& length);

	UINT GetLengthSize();
	UINT GetPictureSetCount();
	BYTE GetProfileCompatibility();	
	void GetProfileLevel(BYTE *profile, BYTE *level);
	UINT GetSequenceSetCount();

	void SetProfileCompatibility(BYTE compatibility);
	void SetProfileLevel(BYTE  profile, BYTE  level);
	
private:
	Mp4SizeTablePropertyPtr GetSizeTable(LPCSTR name);
	
// Data Members -----------------------------------------------
private:
	Mp4AtomPtr fAvcC;
};

};

#endif // !defined(_NS_VISION_MP4_AVCC_H)
