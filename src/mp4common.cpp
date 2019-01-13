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
#include "mp4common.h"
#include "mp4config.h"
#include "mp4property.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Chunk class

Mp4Chunk::Mp4Chunk()
{
	fBuffer				= NULL;
	fDuration			= 0;
	fFristSampleId		= 0;
	fLength				= 0;
	fMaxDuration		= 0;
	fMaxLength			= 0;
	fMaxSampleCount		= 0;
	fSampleCount		= 0;
	fSampleOffset		= 0;
}

Mp4Chunk::~Mp4Chunk()
{
	Clear();
}

int Mp4Chunk::AddSample( MediaFileSample* sample )
{
	if (fSampleCount <= 0) {
		fFristSampleId = sample->GetSampleId();
	}

	fSampleCount++;
	fDuration += (UINT)sample->GetDuration();

	fSamples.Add(sample);
	return 0;
}

/** 清空这个 Chunk 并释放所有的资源. */
void Mp4Chunk::Clear()
{
	if (fBuffer != NULL) {
		Mp4Free(fBuffer);
		fBuffer = NULL;
	}

	fDuration			= 0;
	fFristSampleId		= 0;
	fLength				= 0;
	fMaxLength			= 0;
	fMaxDuration		= 0;
	fMaxSampleCount		= 0;
	fSampleCount		= 0;
	fSampleOffset		= 0;
}

/** 指出这个 Chunk 是否已经满了. */
BOOL Mp4Chunk::IsFull()
{
	if (fSampleCount > fMaxSampleCount) {
		return TRUE;

	} else if (fMaxDuration > 0 && (fDuration > fMaxDuration)) {
		return TRUE;
	}

	return FALSE;
}

/** 重置相关的变量, 开始写新的 Chunk. */
void Mp4Chunk::Reset()
{
	fDuration			= 0;
	fFristSampleId		= 0;
	fLength				= 0;
	fSampleCount		= 0;
	fSampleOffset		= 0;

	fSamples.Clear();
}

/** 
 * 重新分配这个 Chunk 的缓存区的大小. 
 * @param newSize 要分配的新的缓存区大小, 小于当前空间则不会重新分配.
 * @return 返回 0 表示分配成功, 否则返回一个小于 0 的错误码.
 */
int Mp4Chunk::Resize(UINT newSize)
{
	if (newSize > fMaxLength) {			
		if (fMaxLength >= 1024 * 1024 * 4) {
			return MP4_ERR_FAILED;
		}

		// Append sample bytes to chunk buffer
		fMaxLength = newSize;
		fBuffer = (BYTE*)Mp4Realloc(fBuffer, fMaxLength);
	}

	return 0;
}

/**
 * 在指定的位置写入一个表示长度的 32 位整数
 * 
 * @param length 要写入的整数的值
 */
BOOL Mp4Chunk::PutLength(UINT offset, UINT length)
{
	if (fBuffer == NULL) {
		return FALSE;

	} else if (offset + 4 >= fMaxLength) {
		return FALSE;
	}

	BYTE* buffer = fBuffer + offset;
	buffer[0] = (length >> 24) & 0xff;
	buffer[1] = (length >> 16) & 0xff;
	buffer[2] = (length >> 8 ) & 0xff;
	buffer[3] = (length		 ) & 0xff;
	return TRUE;
}

/**
 * 填充新的 Sample 数据.
 * @return 返回 0 表示分配成功, 否则返回一个小于 0 的错误码.
 */
int Mp4Chunk::PutSampleData( BYTE* sampleData, UINT sampleSize )
{
	if (sampleData == NULL || sampleSize <= 0) {
		return MP4_ERR_FAILED; // 无效的参数
	}

	UINT newSize = fLength + sampleSize;
	if (newSize + 8 >= fMaxLength) {			
		int ret = Resize(newSize + 8);
		if (ret < 0) {
			return ret;
		}
	}

	if (fBuffer && newSize <= fMaxLength) {
		memcpy(fBuffer + fLength, sampleData, sampleSize);
		fLength += sampleSize;
	}

	return sampleSize;
}

int Mp4Chunk::PutVideoFragment( BYTE* data, UINT len, BYTE nalHeader )
{
	if (data == NULL || len <= 0) {
		return MP4_ERR_FAILED; // 无效的参数
	}

	// 检查并分配足够的缓存区空间
	int SIZE_HEADER_LEN = 4;
	int size = 0;
	int ret = Resize(len + SIZE_HEADER_LEN + fLength);
	if (ret < 0) {
		return ret;
	}

	// NAL length
	if (nalHeader) {
		// 在 sample 的开始添加 sample 长度头.
		fSampleOffset = GetLength();
		PutLength(fSampleOffset, len);

		fLength += SIZE_HEADER_LEN;
		size    += SIZE_HEADER_LEN;
	}

	// 复制 sample 内容
	ret = PutSampleData(data, len);
	if (ret < 0) {
		return ret;
	}
	size += len;

	// NAL header
	if (nalHeader && fBuffer) {
		BYTE* buffer = fBuffer + fSampleOffset + SIZE_HEADER_LEN;
		*buffer = nalHeader;
	}

	// 如果这是一个分片的一部分(没有Start Code), 则重写 NALU 长度
	UINT sampleSize = fLength - (fSampleOffset + SIZE_HEADER_LEN);
	PutLength(fSampleOffset, sampleSize);

	return size;
}

UINT Mp4Chunk::ReadLength( BYTE* data )
{
	if (data == NULL) {
		return 0;
	}

	UINT length = data[0] << 24;
	length |= data[1] << 16;
	length |= data[2] << 8;
	length |= data[3];
	return length;
}

void Mp4Chunk::SetMaxSampleCount( UINT count )
{
	fMaxSampleCount	= count;
}

void Mp4Chunk::SetMaxDuration( UINT duration )
{
	fMaxDuration		= duration;
}

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Descriptor

/**
 * 构建一个新的 Mp4Descriptor 属性.
 * @param type 这个属性的类型.
 */
Mp4Descriptor::Mp4Descriptor(Mp4DescriptorType type)
{
	fType		= type;
	fSize		= 0;

	if (fType == Mp4ESDescrTag) {
		fProperties.Add(new Mp4Property(PT_INTEGER, 1, "objectTypeId"));
		fProperties.Add(new Mp4Property(PT_INTEGER, 1, "streamType"));
		fProperties.Add(new Mp4Property(PT_INTEGER, 3, "bufferSize"));
		fProperties.Add(new Mp4Property(PT_INTEGER, 4, "maxBitrate"));
		fProperties.Add(new Mp4Property(PT_INTEGER, 4, "avgBitrate"));
	}
}

/**
 * 构建这个属性的内容.
 */
void Mp4Descriptor::Build()
{
	UINT count = 0;
	memset(fBuffer, 0, sizeof(fBuffer));

	if (fType == Mp4ESDescrTag) {
		fBuffer[count++] = Mp4ESDescrTag;
		WriteMpegLength(0x22, count);

		// ESID 
		fBuffer[count++] = 0x00;
		fBuffer[count++] = 0x00;

		// flags
		fBuffer[count++] = 0x00;

		fBuffer[count++] = Mp4DecConfigDescrTag;
		WriteMpegLength(0x14, count);
		WriteInt(MP4_MPEG4_AUDIO_TYPE, 1, count);
		WriteInt(MP4_AUDIOSTREAMTYPE, 1, count);

		// buffer size DB
		WriteInt(GetPropertyValue("bufferSize"), 3, count);	
		WriteInt(GetPropertyValue("maxBitrate"), 4, count);
		WriteInt(GetPropertyValue("avgBitrate"), 4, count);

		fBuffer[count++] = Mp4DecSpecificDescrTag;
		WriteMpegLength(0x02, count);

		fBuffer[count++] = 0x11;
		fBuffer[count++] = 0x90;

		fBuffer[count++] = Mp4SLConfigDescrTag;
		WriteMpegLength(0x01, count);

		fBuffer[count++] = 0x02;

		fSize = count;

	} else if (fType == Mp4FileIODescrTag) {
		fBuffer[count++] = Mp4FileIODescrTag;
		WriteMpegLength(0x07, count);

		fBuffer[count++] = 0x00; 
		fBuffer[count++] = 0x4F; 
		fBuffer[count++] = 0xFF; 
		fBuffer[count++] = 0xFF; 

		fBuffer[count++] = 0x0F; 
		fBuffer[count++] = 0x7F; 
		fBuffer[count++] = 0xFF;

		fSize = count;

	} else {
		fSize = 0;
	}
}

/**
 * 返回指定名称的属性.
 * @param name 属性的名称
 * @return 相关的属性. 
 */
Mp4PropertyPtr Mp4Descriptor::GetProperty( LPCSTR name )
{
	if (name == NULL || *name == '\0') {
		return NULL;
	}

	for (UINT i = 0; i < fProperties.GetCount(); i++) {
		Mp4PropertyPtr property = fProperties[i];
		if (property && property->GetName() 
			&& !strcasecmp(name, property->GetName())) {
			return property;
		}
	}
	
	return NULL;
}

/**
 * 返回指定名称的属性的整型值
 * @param name 属性的名称.
 * @return 这个属性的值. 
 */
UINT Mp4Descriptor::GetPropertyValue( LPCSTR name )
{
	Mp4PropertyPtr property = GetProperty(name);
	if (property) {
		return (UINT)property->GetValueInt();
	}
	return 0;
}

/**
 * 从指定文件的当前位置读取这个属性的内容.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Descriptor::Read( Mp4File* file )
{
	// TODO: 读取 Descriptor 的值.
	return MP4_S_OK;
}

/**
 * 把这个属性的内容写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Descriptor::Write( Mp4File* file )
{
	Build();
	file->WriteBytes(fBuffer, fSize);
	return MP4_S_OK;
}

/** 写入指定位的整数值. */
void Mp4Descriptor::WriteInt( UINT value, UINT size, UINT& count )
{
	for (int i = size - 1; i >= 0; i--) {
		fBuffer[count++] = (value >> (i * 8)) & 0xFF;
	}
}

/**
 * 在指定的位置写入一个 MPEG 风格的长度值.
 * @param length 
 * @param count 
 */
void Mp4Descriptor::WriteMpegLength( UINT length, UINT& count )
{
	fBuffer[count++] = 0x80;	// size
	fBuffer[count++] = 0x80;	// size
	fBuffer[count++] = 0x80;	// size
	fBuffer[count++] = (BYTE)length; // size
}

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// 其他工具方法

/** 返回 MP4 文件形式的当前时间. */
INT64 Mp4Utils::Mp4GetTimestamp()
{
	INT64 ret = time(NULL);
	ret += 2082844800;	// 208284480 is (((1970 - 1904) * 365) + 17) * 24 * 60 * 60
	return ret;	// MP4 start date is 1/1/1904
}

/** 把 4 个 ASCII 字符型式的 atom 类型字符串转换成 UINT 型式的 atom 类型. */
UINT Mp4Utils::AtomId( LPCSTR type )
{
	if (type == NULL || *type == '\0') {
		return 0;
	} 

	UINT id  = (type[0] << 24);
	id		|= (type[1] << 16);
	id		|= (type[2] << 8 );
	id		|= (type[3]);
	return id;
}

UINT Mp4Utils::Mp4AtomType( UINT atomId, char* type )
{
	if (type == NULL) {
		return 0;
	} 

	type[0] = (atomId >> 24);
	type[1] = (atomId >> 16);
	type[2] = (atomId >> 8);
	type[3] = (atomId >> 0);
	type[4] = '\0';
	return atomId;
}

INT64 Mp4Utils::Mp4TimeScale( INT64 duration, UINT timeScale, UINT newTimeScale )
{
	if (timeScale > 0 && timeScale != newTimeScale) {
		duration = (duration * newTimeScale) / timeScale;
	}

	return duration;
}


/**
 * 把指定的十六进制字符转换成相应的整数.
 * @param ch 要转换的十六进制字符, 只能包含 '0' ~ '9', 'A' ~ 'F', 'a' ~ 'f'.
 * @return 返回相应的整数值.
 */
int Mp4Utils::HexChar2Int(char ch) 
{
	if (ch >= '0' && ch <= '9') {
		return ch - '0';

	} else if (ch >= 'a' && ch <= 'f') {
		return 10 + (ch - 'a');

	} else if (ch >= 'A' && ch <= 'F') {
		return 10 + (ch - 'A');
	}
	return -1;
}
/**
 * 把指定的 16 进制表示字符串转换为 2 进制的 BYTE 数组.
 * @param text 要转换的字符串, 如 "FE00FBCFEC"
 * @param buf  缓存区
 * @param buflen 缓存区大小
 * @return 返回成功解析的字节的个数.
 */
int Mp4Utils::HexDecode(LPCSTR text, BYTE* buf, UINT buflen) 
{
	if (text == NULL || buf == NULL || buflen <= 0) {
		return 0;
	}
	const char* p = text;
	int count = 0;
	for (size_t i = 0; i < buflen; i++) {
		if (p[0] == '\0' || p[1] == '\0' ) {
			break;
		}

		int a = HexChar2Int(p[0]);
		int b = HexChar2Int(p[1]);
		if (a < 0 || b < 0) {
			break;
		}

		buf[i] = (BYTE)((a << 4) | b);
		count++;

		p += 2;
	}
	return count;
}

int Mp4Utils::HexEncode( void* value, UINT len, char* buffer, UINT bufLen )
{
	if (value == NULL || len <= 0) {
		return 0;

	} else if (buffer == NULL || bufLen <= 0) {
		return 0;
	}

	int ret = 0;
	char* buf = buffer;
	char* bufEnd = buf + bufLen;

	BYTE* p = (BYTE*)value;
	BYTE* end = p + len;
	while (p < end) {
		sprintf(buf, "%02X", *p);
		ret += 2;
		buf += 2;
		p++;

		if (buf + 2 >= bufEnd) {
			break;
		}
	}

	return ret;
}



};

