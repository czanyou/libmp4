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
#include "stdafx.h"
#include <sys/stat.h>
#include <sys/types.h>

#ifndef _WIN32
#include <sys/mman.h>
#endif

#include "mp4.h"
#include "mp4file.h"
#include "mp4common.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4File

Mp4File::Mp4File()
{
	fBitsBuffer		= 0;
	fBitsCount		= 0;
	fByteCount		= 0;
	fByteBuffer		= NULL;

	fFile			= NULL;
	fFileSize		= 0;
	fIsWriting		= TRUE;

	memset(fFileName, 0, sizeof(fFileName));
}

Mp4File::~Mp4File()
{
	Close();
}

/** 关闭这个文件. */
void Mp4File::Close()
{
	if (fFile) {
		fclose(fFile);
		fFile = NULL;
	}

	if (fByteBuffer) {
		Mp4Free(fByteBuffer);
		fByteBuffer = NULL;
	}

	fIsWriting		= FALSE;
	fFileSize		= 0;
	fBitsBuffer		= 0;
	fBitsCount		= 0;
	fByteCount		= 0;
}

/** 返回当前读写位置. */
INT64 Mp4File::GetPosition()
{
	if (fFile == NULL) {
		return 0;
	}

#ifdef HI3515
	// HI3515 调用 fsetpos 报错
	return ftell(fFile);

#else

	fpos_t fpos;
	if (fgetpos(fFile, &fpos) < 0) {
		return 0;
	}

#ifdef __linux
	INT64 ret = fpos.__pos;
#else
	INT64 ret = fpos;
#endif
	return ret;

#endif

}

/**
 * 返回这个文件的当前长度.
 * @return 文件的长度. 
 */
INT64 Mp4File::GetFileSize()
{
	fFileSize = 0;

#ifdef _WIN32
	int fd = _fileno(fFile);
	fFileSize = _filelength(fd); 

#else
	struct stat s;
	if (fstat(fileno(fFile), &s) < 0) {
		return 0;
	}
	fFileSize	= s.st_size;
#endif
	return fFileSize;
}

/** 
 * 打开或创建指定名称的 MP4 文件. 
 * @param name 文件名称.
 * @param mode 打开模式, 具体参考 fopen 方法.
 */
int Mp4File::Open( LPCSTR name, LPCSTR mode )
{
	if (fFile != NULL) {
		return MP4_ERR_ALREADY_OPEN; // 文件已经打开了

	} else if (name == NULL) {
		return -1;
	}

	if (mode == NULL) {
		mode = "r";
	}

	strncpy(fFileName, name, MAX_PATH);
	fFile = fopen(name, mode);
	if (fFile == NULL) {
		return MP4_ERR_OPEN;
	}

	//setvbuf(fFile, NULL, _IOFBF, 0);

	// 取得文件的实际长度
	if (strstr(mode, "r") != NULL) {
		GetFileSize();
		fIsWriting	= FALSE;

		if (strstr(mode, "r+") != NULL) {
			fIsWriting	= TRUE;
		}

	} else if (strstr(mode, "w") != NULL) {
		fIsWriting	= TRUE;
		fFileSize	= 0;
	}

	fBitsBuffer	= 0;
	fBitsCount	= 0;

	return MP4_S_OK;
}

UINT Mp4File::PreRead( UINT size )
{
	return 0;
}

/** 读取指定个比特. 长度不能超过 8. */
BYTE Mp4File::ReadBits( UINT size )
{
	if (size <= 0 || size >= 8) {
		return 0;
	}

	if (fBitsCount == 0) {
		// 缓存区为空, 则从文件读取一个字节
		fBitsBuffer = ReadInt(1);
		fBitsCount = 8;
	}

	fBitsCount -= size;
	if (fBitsCount <= 0) {
		fBitsCount = 0;
	}

	INT64 ret = fBitsBuffer;
	if (fBitsCount > 0) {
		ret >>= fBitsCount;
	}

	INT64 mask = 0;
	INT64 a = 0x01;
	for (UINT i = 0; i < size; i++) {
		mask |= a << i;
	}

	return BYTE(ret & mask);
}

/** 读取指定个长度的字节, 但不改变文件指针位置. */
UINT Mp4File::PeekBytes( BYTE* bytes, UINT numBytes )
{
	INT64 position = GetPosition();
	UINT size = ReadBytes(bytes, numBytes);
	if (size > 0) {
		SetPosition(position);
	}
	return size;
}

/** 读取指定长度的字节. */
UINT Mp4File::ReadBytes( BYTE* bytes, UINT numBytes )
{
	if (fFile == NULL || bytes == NULL || numBytes <= 0) {
		return 0;
	}

	return fread(bytes, 1, numBytes, fFile);
}

/** 读取一个指定长度的整数. */
UINT Mp4File::ReadInt( UINT size )
{
	if (size <= 0 || size > 4) {
		return 0;
	}

	BYTE data[9];
	UINT read = ReadBytes(data, size);
	if (read != size) {
		return 0;
	}

	return ReadInt(data, size);
}

UINT Mp4File::ReadInt( BYTE * data, UINT size )
{
	UINT ret = 0;
	switch (size) {
	case 1:	
		ret  = data[0];
		break;

	case 2:
		ret = (UINT)((data[0] << 8) | data[1]);
		break;

	case 3:
		ret = (UINT)((data[0] << 16) | (data[1] << 8) | data[2]);
		break;

	case 4:
		ret = (UINT)((data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]);
		break;
	}
	return ret;
}

INT64 Mp4File::ReadInt64( UINT size )
{
	if (size <= 0 || size > 8) {
		return 0;

	} else if (size <= 4) {
		return ReadInt(size);
	}

	BYTE data[9];
	UINT read = ReadBytes(data, size);
	if (read != size) {
		return 0;
	}

	INT64 ret = 0;
	for (UINT i = 0; i < size; i++) {
		ret = ret << 8;
		ret |= data[i];
	}
	return ret;
}

/** 读取 MPEG 风格的长度值. */
UINT Mp4File::ReadMpegLength()
{
	UINT length = 0;
	BYTE numBytes = 0;
	BYTE b = 0;

	do {
		BYTE b = (BYTE)ReadInt(1);
		length = (length << 7) | (b & 0x7F);
		numBytes++;
	} while ((b & 0x80) && numBytes < 4);

	return length;
}

/**
 * fwrite() with automatic retry on syscall interrupt
 * @param	ptr	location to read from
 * @param	size	size of each element of data
 * @param	nmemb	number of elements
 * @param	stream	file stream
 * @return	number of items successfully written
 */
size_t Mp4File::SafeWrite (const void *ptr, size_t size, size_t nmemb, FILE * stream)
{
    size_t ret = 0;
	if (ptr == NULL || size == 0 || nmemb == 0 || stream == NULL) {
		return ret;
	}

	int loop = 0;
    do {
		clearerr (stream);
		ret += fwrite ((char*)ptr + (ret * size), size, nmemb - ret, stream);
		loop++;
		if (loop > 1) {
			usleep(1000);
		} else if (loop > 100) {
			break; // 防止陷入死循环
		}
    } while ((ret < nmemb) && ferror(stream) && (errno == EINTR));
    return ret;
}

/** 设置当前读写位置. */
int Mp4File::SetPosition( INT64 position )
{
	if (fFile == NULL || position < 0) {
		return -1;
	}

#ifdef HI3515
	// HI3515 调用 fsetpos 报错
	return fseek(fFile, (long)position, SEEK_SET);
#endif

	fpos_t fpos;
#ifdef __linux
	fpos.__pos = position;
#else
	fpos = position;
#endif
	return fsetpos(fFile, &fpos);
}

/**
 * 开始读取比特值.
 */
void Mp4File::StartReadBits()
{
	fBitsCount  = 0;
	fBitsBuffer = 0;
}

/** 写入指定长度的字节. */
UINT Mp4File::WriteBytes( BYTE* bytes, UINT numBytes )
{
	if (!fIsWriting || fFile == NULL || bytes == NULL || numBytes <= 0) {
		return 0;
	}

	return SafeWrite(bytes, 1, numBytes, fFile);
}

/** 写入指定个比特. 长度不能超过 8.  */
UINT Mp4File::WriteBits( BYTE value, UINT size )
{
	if (size <= 0 || size >= 8) {
		return 0;
	}
	
	if (fBitsCount == 0) {
		fBitsBuffer = 0;
	}

	INT64 mask = 0;
	INT64 a = 0x01;
	for (UINT i = 0; i < size; i++) {
		mask |= a << i;
	}
	value = value & mask;
	
	fBitsCount += size;
	if (fBitsCount < 8) {
		fBitsBuffer |= value << (8 - fBitsCount);
	} else {
		fBitsBuffer |= value;
	}
	
	if (fBitsCount >= 8) {
		WriteInt(fBitsBuffer, 1);
		fBitsCount = 0;
	}
	
	return 0;
}

/** 写入一个指定长度的整数, 单位为字节. */
UINT Mp4File::WriteInt( INT64 value, UINT size )
{
	if (size <= 0 || size > 8) {
		return 0;
	}

	BYTE data[9];
	switch (size) {
	case 1:	
		data[0] = (BYTE)(value & 0xFF); 
		break;

	case 2:	
		data[0] = (BYTE)(value >> 8); 
		data[1] = (BYTE)(value & 0xFF); 
		break;

	case 3:	
		data[0] = (BYTE)(value >> 16);
		data[1] = (BYTE)(value >> 8);
		data[2] = (BYTE)(value & 0xFF);
		break;

	case 4: 
		data[0] = (BYTE)(value >> 24);
		data[1] = (BYTE)(value >> 16);
		data[2] = (BYTE)(value >> 8);
		data[3] = (BYTE)(value & 0xFF);		 
		break;

	default: 
		{
			for (UINT i = 0; i < size; i++) {
				data[size - i - 1] = (BYTE)value;
				value = value >> 8;
			}
		}
		break;
	}
	
	return WriteBytes(data, size);
}

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4MapFile class

Mp4MapFile::Mp4MapFile()
{
	fPosition	= 0;
	fFile		= -1;
	fFileData	= NULL;
	fFileSize	= 0;
}

void Mp4MapFile::Close()
{
#ifndef _WIN32
	if (fFileData) {
		munmap(fFileData, fFileSize);
		fFileData = NULL;
		fFileSize = 0;
	}
#endif

	if (fFile > 0) {
		close(fFile);
		fFile = -1;
	}
}

INT64 Mp4MapFile::GetFileSize()
{
	return fFileSize;
}

INT64 Mp4MapFile::GetPosition()
{
	return fPosition;
}

int Mp4MapFile::Open( LPCSTR name, LPCSTR mode )
{
	Close();

#ifndef _WIN32

	if ((fFile = open(name, O_RDONLY)) == -1) {
		//LOG_E("can't open file %s\n", name);
		return -1;
	}

	struct stat statbuf;
	//ASSERT(fstat(fFile, &statbuf) == 0);

	BYTE* fileData = (BYTE*)mmap(0, statbuf.st_size, PROT_READ, MAP_SHARED, fFile, 0);
	if (fileData == MAP_FAILED ) {
		//LOG_E("mmap error for fFile.\n");
		return -1;
	}

	fFileSize = statbuf.st_size;
	fFileData = fileData;
	return 0;

#else
	return -1;
#endif
}

UINT Mp4MapFile::ReadBytes( BYTE* bytes, UINT numBytes )
{
	if (bytes == NULL || numBytes <= 0) {
		return 0;

	} else if (fFileData == NULL) {
		return 0;

	} else if (fPosition < 0 || fPosition >= fFileSize) {
		return 0;

	} else if (fPosition + numBytes >= fFileSize) {
		numBytes = UINT(fFileSize - fPosition);
	}

	memcpy(bytes, fFileData + fPosition, numBytes);
	fPosition += numBytes;
	return numBytes;
}

int Mp4MapFile::SetPosition( INT64 position )
{
	fPosition = position;
	return MP4_S_OK;
}

UINT Mp4MapFile::WriteBytes( BYTE* bytes, UINT numBytes )
{
	return 0;
}

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4MemFile class

Mp4MemFile::Mp4MemFile( BYTE* buffer, INT64 size )
{
	fPosition		= 0;
	fFileSize		= 0;
	fByteCount		= size;
	fBuffer			= buffer;
}

INT64 Mp4MemFile::GetFileSize()
{
	return fFileSize;
}

INT64 Mp4MemFile::GetPosition()
{
	return fPosition;
}

int Mp4MemFile::Open( LPCSTR name, LPCSTR mode )
{
	StartReadBits();
	return 0;
}

UINT Mp4MemFile::ReadBytes( BYTE* bytes, UINT numBytes )
{
	if (fPosition + numBytes >= fByteCount) {
		return 0;
	}

	memcpy(bytes, fBuffer + fPosition, numBytes);
	fPosition += numBytes;
	return numBytes;
}

void Mp4MemFile::SetFileSize( INT64 size )
{
	fFileSize = size;
}

int Mp4MemFile::SetPosition( INT64 position )
{
	fPosition = position;
	return 0;
}

UINT Mp4MemFile::WriteBytes( BYTE* bytes, UINT numBytes)
{
	if (fPosition + numBytes >= fByteCount) {
		return 0;
	}

	memcpy(fBuffer + fPosition, bytes, numBytes);
	fPosition += numBytes;
	if (fFileSize < fPosition) {
		fFileSize = fPosition;
	}

	//TRACE("%u (pos:%u)\r\n", (UINT)fFileSize, fPosition);
	return numBytes;
}

}
