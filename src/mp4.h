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
#ifndef _NS_VISION_MP4_STREAM_H
#define _NS_VISION_MP4_STREAM_H

#include "base/base_types.h"
#include "base/base_object.h"

using namespace core;

#define VisionBaseObject BaseObject

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// MediaTrackType enum

/**  Track 类型. */
enum MediaTrackType
{
	MEDIA_TRACK_AUDIO	= 0,
	MEDIA_TRACK_VIDEO	= 1,
	MEDIA_TRACK_TEXT	= 2,
	MEDIA_TRACK_GPS		= 10
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// MeidaFileSample class

/**
 * 代表了媒体文件的一个 Sample (样本) 对象.
 * 
 * Sample 通常是一个完整的视频或音频帧.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class MediaFileSample : public VisionBaseObject
{
public:
	MediaFileSample();
	MediaFileSample(const MediaFileSample& ths);
	virtual ~MediaFileSample();
	VISION_BASEREF_METHODS(MediaFileSample);		///< 实现引用计数功能.
	MediaFileSample& operator = (const MediaFileSample& ths);

// Attributes -------------------------------------------------
public:
	BYTE* GetData()		 const	{ return fData;			} ///< 这个 Sample 的数据内容
	INT64 GetDuration()  const	{ return fDuraction;	} ///< 这个 Sample 的时间长度
	UINT  GetMaxSize()	 const	{ return fMaxSize;		} ///< 这个 Sample 的缓存区的最大长度
	UINT  GetSampleId()  const	{ return fSampleId;		} ///< 这个 Sample 的 ID.
	UINT  GetSize()		 const	{ return fSize;			} ///< 这个 Sample 的数据内容的长度
	UINT  GetTimeScale() const	{ return fTimeScale;	} ///< 这个 Sample 的时间比
	INT64 GetTimestamp() const	{ return fTimestamp;	} ///< 这个 Sample 的时间戳
	UINT  GetTrackIndex() const	{ return fTrackIndex;	} ///< 这个 Sample 所属的 Track 的 ID

	BOOL  IsSyncPoint()	 const	{ return fIsSyncPoint;	} ///< 指出这个 Sample 是否是一个同步点
	BOOL  IsEnd()		 const  { return fIsEnd;		} ///< 指出这个 Sample 是否是当前帧最后一个 Sample

	void SetData(BYTE* val)		{ fData			= val;	} ///< 设置这个 Sample 的数据内容
	void SetDuration(INT64 val) { fDuraction	= val;	} ///< 设置这个 Sample 的时间长度
	void SetMaxSize(UINT val)	{ fMaxSize		= val;	} ///< 设置这个 Sample 的缓存区的最大长度
	void SetSampleId(UINT val)	{ fSampleId		= val;	} ///< 设置这个 Sample 的 ID.
	void SetSize(UINT val)		{ fSize			= val;	} ///< 设置这个 Sample 的数据内容的长度
	void SetSyncPoint(BOOL val)	{ fIsSyncPoint	= val;	} ///< 设置这个 Sample 是否是一个同步点
	void SetTimeScale(UINT val) { fTimeScale	= val;	} ///< 设置这个 Sample 的时间比
	void SetTimestamp(INT64 val){ fTimestamp	= val;	} ///< 设置这个 Sample 的时间戳
	void SetTrackIndex(UINT val){ fTrackIndex	= val;	} ///< 设置这个 Sample 所属的 Track 的 ID
	void SetEnd(BOOL val)		{ fIsEnd		= val;	} ///< 设置这个 Sample 是否是当前帧最后一个 Sample

public:
	void Clear();

// Data Members -----------------------------------------------
private:
	BYTE* fData;			///< 这个 Sample 的数据内容
	INT64 fDuraction;		///< 这个 Sample 的时间长度
	BOOL  fIsEnd;			///< 这个 Sample 是否最后一个Sample
	BOOL  fIsSyncPoint;			///< 这个 Sample 是否是一个同步点
	UINT  fMaxSize;			///< 这个 Sample 的缓存区的最大长度
	UINT  fSampleId;		///< 这个 Sample 的 ID.
	UINT  fSize;			///< 这个 Sample 的数据内容的长度
	UINT  fTimeScale;		///< 这个 Sample 的时间比
	INT64 fTimestamp;		///< 这个 Sample 的时间戳
	UINT  fTrackIndex;			///< 这个 Sample 所属的 Track 的 ID
};

/** MediaFileSample 智能指针类型. */
typedef SmartPtr<MediaFileSample> MediaFileSamplePtr;

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// MediaFileFormat

class MediaFileFormat : public VisionBaseObject
{
public:
	VISION_BASEREF_METHODS(MediaFileFormat);

public:
	///< 返回这个 Track 的编码格式
	virtual LPCSTR GetCodecName() = 0;

	///< 返回这个 Track 的类型
	virtual LPCSTR GetMediaType() = 0;

	///< 返回这个 Track 的码率
	virtual UINT GetAvgBitrate()  = 0;

	virtual UINT GetChannels()   = 0;

	///< 返回这个 Track 的帧率
	virtual UINT GetFrameRate()   = 0;

	///< 返回这个 Track 的 TimeScale
	virtual UINT GetTimeScale()  = 0;

	///< 返回这个 Track 的视频高度, 单位为像素
	virtual UINT GetVideoHeight() = 0;

	///< 返回这个 Track 的视频宽度, 单位为像素
	virtual UINT GetVideoWidth()  = 0;

	/** 返回这个 Track 的参数集. */
	virtual BYTE* GetParamSets( int type, int index, WORD& length ) = 0;

};

/** MediaFileFormat 智能指针类型. */
typedef SmartPtr<MediaFileFormat> MediaFileFormatPtr;

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// MediaExtractor

/**
 * 代表一个媒体文件阅读器.
 * MediaExtractor facilitates extraction of demuxed, typically encoded, media 
 * data from a data source.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class MediaExtractor : public VisionBaseObject
{
public:
	VISION_BASEREF_METHODS(MediaExtractor);

// Attributes -------------------------------------------------
public:
	///< 媒体创建时间, 1970-1-1 以来经过的秒数
	virtual INT64 GetCreationTime() = 0; 

	///< 媒体时间长度, 单位为毫秒
	virtual INT64 GetDuration()		= 0; 

	virtual INT64 GetTimeLoaded(INT64 bytesLoaded) = 0;

	/** 
	 * 当前流的轨道数量.
	 * Count the number of tracks found in the data source.
	 */
	virtual UINT  GetTrackCount()	= 0; 
	
	/**
	 * Get the track format at the specified index. 
	 */
	virtual MediaFileFormatPtr GetTrackFormat(int trackType) = 0;

	/**
	 * Retrieve the current encoded sample and store it in the byte buffer 
	 * starting at the given offset.
	 */
	virtual MediaFileSample* GetSample() = 0;	

	virtual BOOL IsEndOfFile() = 0;

// Operations -------------------------------------------------
public:
	/**
	 * Advance to the next sample. Returns false if no more sample data is 
	 * available (end of stream).
	 */
	virtual BOOL Advance() = 0;	

	/** 关闭 */
	virtual void Close() = 0;	

	/** 打开指定的媒体文件 */
	virtual int  Open(LPCSTR filename, UINT flags = 0) = 0; 

	/**
	 * All selected tracks seek near the requested time according to the specified mode.
	 */
	virtual void SeekTo(INT64 position) = 0;
};

/** MediaExtractor 智能指针类型. */
typedef SmartPtr<MediaExtractor> MediaExtractorPtr;

/** 创建一个新的 MediaExtractor 对象. */
MediaExtractorPtr MediaCreateExtractor();

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// MediaMuxer

/**
 * 代表一个媒体文件书写器.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class MediaMuxer : public VisionBaseObject
{
public:
	VISION_BASEREF_METHODS(MediaMuxer);

// Attributes -------------------------------------------------
public:
	/** 返回当前媒体文件创建的时间, 单位为 1970 年以来经过的秒数. */
	virtual INT64 GetCreationTime()	= 0;

	/** 返回当前媒体文件的长度, 单位为毫秒. */
	virtual INT64 GetDuration()		= 0;

	/** 返回当前媒体文件的长度, 单位为字节. */
	virtual INT64 GetLength()		= 0;

	/** 指出当前书写器是否已经打开. */
	virtual BOOL  IsOpen()			= 0;

// Operations -------------------------------------------------
public:
	virtual int AddGpsTrackData(LPCSTR name, LPCSTR track) = 0;

	/** 开始写媒体数据. */
	virtual int BeginWriting()		= 0;

	/** 关闭这个书写器. */
	virtual int Close()				= 0;

	/** 结束写媒体数据. */
	virtual int EndStreaming()		= 0;

	/** 结束写媒体数据. */
	virtual int EndWriting()		= 0;

	/** 打开指定的文件. */
	virtual int Open(UINT flags = 0) = 0;

	/** 
	 * 设置音频 Track 的属性. 
	 * @param timeScale 这个Track 的 Time Scale
	 * @param duration 
	 * @param channels 这个Track 的音频通道数
	 * @param type 这个Track 的音频编码类型
	 * @param param 这个Track 的音频编码参数.
	 */
	virtual int SetAudioFormat(UINT timeScale, INT64 duration, int channels, 
		LPCSTR type, int param = 0) = 0;

	/** 设置 GPS 轨迹属性和信息. */
	virtual int SetGpsTrack(LPCSTR track) = 0;

	/** 设置要输出的文件的名称. */
	virtual int SetOutputFilename(LPCSTR filename) = 0;

	/** 
	 * 设置视频 Track 的属性. 
	 * @param timeScale 这个Track 的 Time scale
	 * @param duration duration
	 * @param width 这个Track 的视频宽度
	 * @param height 这个Track 的视频高度
	 * @param type 这个Track 的编码类型, 目前只支持 H.264
	 */
	virtual int SetVideoFormat(UINT timeScale, INT64 duration, WORD width, 
		WORD height, LPCSTR type) = 0;

public:
	/** 
	 * 写入一个音频 Sample. 
	 * @param sample 要写入的 Sample
	 * @return 如果成功则返回 MP4_S_OK. 
	 */
	virtual int WriteAudioSample(MediaFileSample* sample) = 0;

	/** 
	 * 写入一个视频 Sample. 
	 * @param sample 这个 Sample 的内容
	 * @return 如果成功则返回 MP4_S_OK. 
	 */
	virtual int WriteVideoSample(MediaFileSample* sample) = 0;
};

/** MediaMuxer 智能指针类型. */
typedef SmartPtr<MediaMuxer> MediaMuxerPtr;

/** 创建一个新的 MediaMuxer 对象. */
MediaMuxerPtr MediaCreateMuxer();

}

#endif // _NS_VISION_MP4_STREAM_H

