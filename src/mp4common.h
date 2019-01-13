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
#ifndef _NS_VISION_MP4_COMMON_H
#define _NS_VISION_MP4_COMMON_H

#include "time.h"
#include "mp4config.h"
#include "mp4file.h"

/**
 * MP4 文件解复用模块.
 * 这个模块主要用于生成或读取 ISO MP4 格式的多媒体文件.
 * @author ChengZhen (anyou@msn.com)
 */
namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4ErrorCode

/** MP4 模块错误返回码. */
enum Mp4ErrorCode
{
	MP4_S_OK					= 0,	///< 执行成功
	MP4_ERR_FAILED				= -1,	///< 一般性错误
	MP4_ERR_ALREADY_OPEN		= -10,	///< 文件已经打开
	MP4_ERR_OPEN				= -11,	///< 文件打开错误
	MP4_ERR_NOT_OPEN			= -12,	///< 文件还没有打开
	MP4_ERR_NULL_ATOM			= -13,	///< 碰到空的 atom
	MP4_ERR_NULL_PROPERTY		= -14,	///< 碰到空的属性
	MP4_ERR_WRITE				= -15,	///< 发生写错误
	MP4_ERR_READ				= -16,	///< 发生读错误
	MP4_ERR_NULL_FILE			= -17,	///< 碰到空文件指针
	MP4_ERR_ATOM_TOO_LARGE		= -18,
	MP4_ERR_PROPERTY_TOO_LARGE	= -19,
	MP4_ERR_INVALID_LENGTH		= -20,
	MP4_ERR_INVALID_PARAM		= -21,
	MP4_ERR_OUT_OF_MEMORY		= -22
};

enum Mp4ConfigEnum
{
	MP4_ATOM_HEADER_SIZE	= 8,
	MP4_MAX_ATOM_SIZE		= 1024 * 1024 * 2, // 1024 * 1024
	MP4_MAX_STRING_LENGTH	= 64 * 1024, // 字符串属性最大长度
	MP4_MAX_TABLE_LENGTH	= 640 * 1024, // 字符串属性最大长度
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4AudioType

#if 0
enum Mp4AudioType {
	MP4_INVALID_AUDIO_TYPE				= 0x00,
	MP4_MPEG1_AUDIO_TYPE				= 0x6B,
	MP4_MPEG2_AUDIO_TYPE				= 0x69,
	MP4_MPEG2_AAC_MAIN_AUDIO_TYPE		= 0x66,
	MP4_MPEG2_AAC_LC_AUDIO_TYPE			= 0x67,
	MP4_MPEG2_AAC_SSR_AUDIO_TYPE		= 0x68,
	MP4_MPEG4_AUDIO_TYPE				= 0x40,
	MP4_PRIVATE_AUDIO_TYPE				= 0xC0,
	MP4_PCM16_LITTLE_ENDIAN_AUDIO_TYPE	= 0xE0,	/* a private definition */
	MP4_VORBIS_AUDIO_TYPE				= 0xE1,	/* a private definition */
	MP4_AC3_AUDIO_TYPE					= 0xE2,	/* a private definition */
	MP4_ALAW_AUDIO_TYPE					= 0xE3,	/* a private definition */
	MP4_ULAW_AUDIO_TYPE					= 0xE4,	/* a private definition */
	MP4_G723_AUDIO_TYPE					= 0xE5, /* a private definition */
	MP4_PCM16_BIG_ENDIAN_AUDIO_TYPE		= 0xE6, /* a private definition */
};
#endif

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Array class

/** 
 * 代表一个简单的数组模板类. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
template <class T> 
class Mp4Array
{
public:
	Mp4Array();
	virtual ~Mp4Array();

// Operations -------------------------------------------------
public:
	inline UINT GetCount() { return fCount; }		///< 返回这个数组包含的元素的个数
	inline UINT GetMaxSize()  { return fMaxCount; }	///< 返回这个数组当前的最大容量

public:
	inline bool ValidIndex(const UINT index);
	void Resize(UINT size);
	void Add(const T& item);
	void Insert(const T& item, UINT index);
	void Clear();
	T*   GetBuffer() { return fItems; }

	/** 重载 [] 操作符. */
	const T& operator[] (const UINT index) const { 
		if (!ValidIndex(index)) {
			return fEmptyItem;
		}
		return fItems[index];
	}

	/** 重载 [] 操作符. */
	T& operator[] (const UINT index) {
		if (!ValidIndex(index)) {
			return fEmptyItem;
		}
		return fItems[index];
	} 

// Data Members -----------------------------------------------
private:
	T  fEmptyItem;		///< 
	T* fItems;			///< 元素列表
	UINT fCount;		///< 有效元素的数量
	UINT fMaxCount;		///< 最大元素的数量
};

/** 构建一个新的 Mp4Array 数组对象. */
template <class T>
Mp4Array<T>::Mp4Array()
{
	fItems = NULL;
	fCount = 0;
	fMaxCount = 0;
}

template <class T>
Mp4Array<T>::~Mp4Array()
{
	if (fItems) {
		delete[] fItems;
		fItems = NULL;
	}
}

/**
 * 检查指定的索引值是否有效.
 * @param index 要检测的索引.
 * @return 如果有效则返回 TRUE, 否则返回 FALSE. 
 */
template <class T>
inline bool Mp4Array<T>::ValidIndex(const UINT index) {
	if (index >= fCount) {
		return false;
	}
	return true;
}

/** 重新分配数组存储空间. */
template <class T>
void Mp4Array<T>::Resize(UINT size)
{
	if (size < 2) {
		size = 2;

	} else if (size < fCount) {
		size = fCount * 2;
	}

	fMaxCount = size;
	T* newItems = new T[fMaxCount];
	T* oldItems = fItems;
	fItems = NULL;

	if (oldItems) {
		for (UINT i = 0; i < fCount; i++) {
			newItems[i] = oldItems[i];
		}
		delete[] oldItems;
	}
	
	fItems = newItems;
}

/** 
 * 添加一个新的元素. 
 *
 * @param item 要添加的新元素.
 */
template <class T>
void Mp4Array<T>::Add(const T& item) 
{
	if (fMaxCount <= fCount) {
		Resize(fMaxCount * 2);
	}
	
	if (fItems && fMaxCount > fCount) {
		fItems[fCount++] = item;
	}
}

/**
 * 插入一个元素
 *
 * @param item 要插入的新元素.
 * @param index 要插入的位置.
 */
template <class T>
void Mp4Array<T>::Insert( const T& item, UINT index )
{
	if (fMaxCount <= fCount) {
		Resize(fMaxCount * 2);
	}

	if (fItems == NULL) {
		return;
	}

	if (index >= fCount) {
		fItems[fCount++] = item;
		return;
	}

	for (UINT i = fCount; i > index; i--) {
		fItems[i] = fItems[i - 1];
	}
	fItems[index] = item;
	fCount++;
}

/** 清空这个数组的所有元素. */
template <class T>
void Mp4Array<T>::Clear() 
{
	fCount		= 0;
	fMaxCount   = 0;

	T* oldItems = fItems;
	fItems = NULL;

	if (oldItems) {
		delete[] oldItems;
	}
}

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Descriptor class

/** 
 * MP4 Descriptor 类型. 主要用于 "iods,esds" atom 节点 
 *
 */
enum Mp4DescriptorType 
{
	Mp4ODescrTag				= 0x01, 
	Mp4IODescrTag				= 0x02, 
	Mp4ESDescrTag				= 0x03, 
	Mp4DecConfigDescrTag		= 0x04, 
	Mp4DecSpecificDescrTag		= 0x05, 
	Mp4SLConfigDescrTag		 	= 0x06, 
	Mp4ContentIdDescrTag		= 0x07, 
	Mp4SupplContentIdDescrTag 	= 0x08, 
	Mp4IPIPtrDescrTag		 	= 0x09, 
	Mp4IPMPPtrDescrTag		 	= 0x0A, 
	Mp4IPMPDescrTag			 	= 0x0B, 
	Mp4RegistrationDescrTag	 	= 0x0D, 
	Mp4ESIDIncDescrTag			= 0x0E, 
	Mp4ESIDRefDescrTag			= 0x0F, 
	Mp4FileIODescrTag			= 0x10, 
	Mp4FileODescrTag			= 0x11, 
	Mp4ExtProfileLevelDescrTag 	= 0x13, 
	Mp4ExtDescrTagsStart		= 0x80, 
	Mp4ExtDescrTagsEnd			= 0xFE, 
};

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// 其他工具方法

class Mp4Utils 
{
public:
	static UINT  AtomId(LPCSTR type);
	static UINT  Mp4AtomType(UINT atomId, char* type);
	static INT64 Mp4TimeScale(INT64 duration, UINT timeScale, UINT newTimeScale);
	static INT64 Mp4GetTimestamp();
	static int   HexChar2Int(char ch);
	static int   HexDecode(LPCSTR text, BYTE* buf, UINT buflen);
	static int   HexEncode( void* value, UINT len, char* buffer, UINT bufLen );
};

typedef Mp4Array<MediaFileSamplePtr> MediaFileSampleArray;


//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Chunk class

/**
 * 表示一个 Track 中多个 Sample 的集合.
 * A contiguous set of samples for one track. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Chunk
{
public:
	Mp4Chunk();
	~Mp4Chunk();

// Attributes -------------------------------------------------
public:
	MediaFileSampleArray& GetSamples() { return fSamples; }
	BYTE* GetBuffer()		{ return fBuffer;			}
	UINT  GetDuration()		{ return fDuration;			}
	UINT  GetFirstSample()	{ return fFristSampleId;	}
	UINT  GetLength()		{ return fLength;			}
	UINT  GetMaxLength()	{ return fMaxLength;		}
	UINT  GetSampleCount()	{ return fSampleCount;		}
	UINT  GetSampleOffset()	{ return fSampleOffset;		}

	BOOL  IsFull();
	void  SetMaxDuration(UINT duration);
	void  SetMaxSampleCount(UINT count);

// Operations -------------------------------------------------
public:
	void Reset();
	void Clear();
	int  Resize(UINT newSize);
	int  AddSample( MediaFileSample* sample );
	int  PutSampleData(BYTE* data, UINT len);
	int  PutVideoFragment(BYTE* data, UINT len, BYTE nalHeader);
	UINT ReadLength(BYTE* data);

protected:
	BOOL PutLength(UINT offset, UINT length);

// Data Members -----------------------------------------------
private:
	BYTE* fBuffer;			///< 这个 Chunk 的数据缓存区
	UINT  fDuration;		///< 这个 Chunk 当前包含的 Sample 的时间长度
	UINT  fFristSampleId;	///< 这个 Chunk 的第一个 Sample 的 ID
	UINT  fLength;			///< 这个 Chunk 当前包含的数据的大小
	UINT  fMaxDuration;		///< 这个 Chunk 可包含的 Sample 的最大时长
	UINT  fMaxLength;		///< 这个 Chunk 的缓存区的最大大小
	UINT  fMaxSampleCount;  ///< 这个 Chunk 可包含的 Sample 的最大数量
	UINT  fSampleCount;		///< 这个 Chunk 当前包含的 sample 的数目
	UINT  fSampleOffset;	///< 当前 Sample 在 Chunk 的开始位置

	MediaFileSampleArray fSamples;
};


};

#endif // !defined(_NS_VISION_MP4_COMMON_H)
