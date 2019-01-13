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
#ifndef _NS_VISION_MP4_STREAM_READER_H_
#define _NS_VISION_MP4_STREAM_READER_H_

#include "mp4common.h"
#include "mp4atom.h"
#include "mp4reader.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4StreamReader


class Mp4StreamReader : public VisionBaseObject
{
public:
	Mp4StreamReader();
	~Mp4StreamReader();
	VISION_BASEREF_METHODS(Mp4StreamReader);

// Attributes -------------------------------------------------
public:
	INT64 GetPosition() { return fPosition;	}	///< 返回当前的读取位置
	INT64 GetFileSize() { return fFileSize; }	///< 返回当前的读取位置
	void  SetPosition(INT64 position);

// Operations -------------------------------------------------
public:
	int Open(LPCSTR name);
	void Close();
	UINT ReadBytes(BYTE* bytes, UINT numBytes);
	char* ReadTrackFile(LPCSTR filename);

// Data Members -----------------------------------------------
protected:
	Mp4FilePtr	fMp4File;		///< MP4 文件
	INT64		fFileSize;
	INT64		fMdatOffset;
	INT64		fPosition;
	BYTE*		fHeader;
	UINT		fHeaderSize;
	BOOL        fIsOpen;
};

};

#endif // !defined(_NS_VISION_MP4_STREAM_READER_H_)
