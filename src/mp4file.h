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

#ifndef _NS_VISION_MP4_FILE_H
#define _NS_VISION_MP4_FILE_H

#include "mp4.h"

namespace mp4 {


//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4File

/** 
 * Mp4File 代表一个 MP4 文件, 这个类主要封装了 MP4 文件的底层 I/O 操作. 
 * 
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4File : public VisionBaseObject
{
public:
	Mp4File();
	virtual ~Mp4File();
	VISION_BASEREF_METHODS(Mp4File);	///< 实现引用计数功能

// Operations -------------------------------------------------
public:
	virtual int   Open(LPCSTR name, LPCSTR mode);
	virtual void  Close();

	virtual INT64 GetPosition();
	virtual int   SetPosition(INT64 position);
	virtual INT64 GetFileSize();

	virtual UINT  ReadBytes (BYTE* bytes, UINT numBytes);
	virtual UINT  WriteBytes(BYTE* bytes, UINT numBytes);
	
public:
	static UINT ReadInt( BYTE * data, UINT size );

	UINT  PreRead(UINT size);
	UINT  PeekBytes(BYTE* bytes, UINT numBytes);
	UINT  ReadInt  (UINT size);

	INT64 ReadInt64(UINT size);
	BYTE  ReadBits (UINT size);
	UINT  ReadMpegLength();

	UINT  WriteInt (INT64 value, UINT size);
	UINT  WriteBits(BYTE value, UINT size);
	void  StartReadBits();

protected:
	size_t SafeWrite (const void *ptr, size_t size, size_t nmemb, FILE * stream);

// Data Members -----------------------------------------------
private:
	INT64 fBitsBuffer;		///< 比特数据缓存区
	int   fBitsCount;		///< 缓存的比特数.

	BYTE* fByteBuffer;		///< 
	UINT  fByteCount;		///< 

	FILE* fFile;			///< 文件句柄
	char  fFileName[MAX_PATH + 1];
	INT64 fFileSize;		///< 文件长度

	BOOL  fIsWriting;		///< 是否正在写
};

/** Mp4File 智能指针类型. */
typedef SmartPtr<Mp4File> Mp4FilePtr;



//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4MapFile class

/** 
 * Mp4MapFile 代表一个内存映射 MP4 文件. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4MapFile : public Mp4File
{
public:
	Mp4MapFile();

// Attributes -------------------------------------------------
public:
	INT64 GetFileSize();
	INT64 GetPosition();
	int   SetPosition(INT64 position);

// Operations -------------------------------------------------
public:
	int Open(LPCSTR name, LPCSTR mode);
	void Close();
	UINT ReadBytes (BYTE* bytes, UINT numBytes);
	UINT WriteBytes(BYTE* bytes, UINT numBytes);

// Data Members -----------------------------------------------
private:
	INT64 fPosition;
	BYTE* fFileData;
	UINT  fFileSize;
	int	  fFile;
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4MemFile class

/**
 * Mp4MemFile 代表一个内存文件.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4MemFile : public Mp4File
{
public:
	Mp4MemFile(BYTE* buffer, INT64 size);

// Operations -------------------------------------------------
public:
	virtual INT64 GetPosition();
	virtual int   SetPosition(INT64 position);
	virtual void  SetFileSize(INT64 size);
	virtual INT64 GetFileSize();
	virtual int Open(LPCSTR name, LPCSTR mode);
	virtual void  Close() {}
	virtual UINT ReadBytes (BYTE* bytes, UINT numBytes);
	virtual UINT WriteBytes(BYTE* bytes, UINT numBytes);

// Data Members -----------------------------------------------
private:
	INT64 fPosition;	///< 当前读写位置
	INT64 fFileSize;	///< 当前文件大小
	INT64 fByteCount;	///< 缓存区的大小
	BYTE* fBuffer;		///< 缓存区
};

}

#endif // _NS_VISION_MP4_FILE_H
