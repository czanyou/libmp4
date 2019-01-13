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
#ifndef _NS_VISION_MP4_READER_H_
#define _NS_VISION_MP4_READER_H_

#include "mp4common.h"
#include "mp4atom.h"
#include "mp4track.h"
#include "mp4stream.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Reader

/** 
 * 代表一个 MP4 文件解复用器(Reader). 
 * 这个类用于读取 MP4 文件的内容. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Reader : public MediaExtractor
{
public:
	Mp4Reader();
	virtual ~Mp4Reader();

// Attributes -------------------------------------------------
public:
	MediaFileFormatPtr GetTrackFormat(int trackType);
	Mp4TrackPtr GetTrack( LPCSTR type );
	Mp4TrackPtr GetTrack( UINT index );
	Mp4FilePtr GetFile();	///< 返回相关的 Mp4File 对象
	Mp4Stream& GetStream();

	INT64 GetCreationTime();
	INT64 GetDuration();
	INT64 GetPosition();		///< 返回当前的读取位置
	INT64 GetTimeLoaded(INT64 bytesLoaded);
	UINT  GetTimeScale();
	UINT  GetTrackCount();	

	UINT  GetSampleCount(int trackType);
	UINT  GetSampleId(int trackType);

	BOOL  IsEndOfFile();


// Operations -------------------------------------------------
public:
	virtual int  Open( LPCSTR name,   UINT flags = 0 );
	virtual int  Open( Mp4File* file, UINT flags = 0 );
	virtual void Close();

public:
	BOOL Advance();
	MediaFileSample* GetSample();

	MediaFileSample* GetAudioSample( UINT sampleId );
	MediaFileSample* GetVideoSample( UINT sampleId );

	MediaFileSample* GetNextGpsSample();				
	MediaFileSample* GetNextSyncAudioSample();

public:
	void  Clear();

	int   ReadSample( Mp4TrackPtr track, UINT sampleId, MediaFileSample* sample );
	int   ReadSample( Mp4TrackPtr track, UINT sampleId, MediaFileSample* sample, INT64* offset = NULL);

	int   LoadMoovAtom();
	void  SeekTo(INT64 position);
	void  UpdateChunksOffset();

// Implementation ---------------------------------------------
private:
	int   LoadTracks();
	int   LoadMvhdAtom();
	INT64 GetSampleTime(INT64 timestamp, UINT timeScale);

// Data Members -----------------------------------------------
private:
	MediaFileSample	fAudioSample;	///< Current audio sample
	MediaFileSample	fTraceSample;	///< Current video sample
	MediaFileSample	fVideoSample;	///< Current track sample
	MediaFileSample* fNextSample;
	Mp4TrackArray fTracks;		///< Track 列表

	Mp4Stream fMp4Stream;		///< 

	BYTE*	fAudioBuffer;		///< 音频数据缓存区
	UINT	fAudioBufferMaxSize;///< 音频数据缓存区的最大长度
	char    fAudioFormat[64];	///< 音频编码格式
	UINT	fAudioSampleId;		///< 下一次要读取的音频帧的 ID.
	INT64	fAudioSampleOffset;	///< 
	int		fAudioTrackIndex;	///< 音频 Track 的索引值

	INT64	fPosition;			///< 当前播放的位置, 单位为毫秒

	BYTE*	fVideoBuffer;		///< 视频数据缓存区
	UINT	fVideoBufferMaxSize;///< 视频数据缓存区的最大长度
	UINT	fVideoSampleId;		///< 下一次要读取的视频帧的 ID.
	INT64	fVideoSampleOffset;	///< 
	int		fVideoTrackIndex;	///< 视频 Track 的索引值
};

typedef SmartPtr<Mp4Reader> Mp4ReaderPtr;

};

#endif // !defined(_NS_VISION_MP4_READER_H_)
