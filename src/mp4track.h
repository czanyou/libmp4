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
#ifndef _NS_VISION_MP4_TRACK_H
#define _NS_VISION_MP4_TRACK_H

#include "mp4common.h"
#include "mp4atom.h"
#include "mp4table.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4SampleDescription


/** 
 * A structure which defines and describes the format of some number of samples 
 * in a track
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4SampleDescription
{
public:
	Mp4SampleDescription();

};


//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Track

/** 
 * 代表 MP4 文件的一个轨道 (Track). 
 * A collection of related samples (q.v.) in an ISO base media file. For media 
 * data, a track corresponds to a sequence of images or sampled audio. For hint 
 * tracks, a track corresponds to a streaming channel.
 * Track 是 MP4 文件的主要组成部分, 代表了一路音频或视频等类型的媒体数据
 * 以及相关参数.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Track : public MediaFileFormat
{
public:
	Mp4Track();
	virtual ~Mp4Track();
	VISION_BASEREF_METHODS(Mp4Track);	///< 实现引用计数功能.

// Attributes -------------------------------------------------
public:
	Mp4SampleTable& GetSampleTable();
	Mp4AtomPtr GetTrackAtom();
	Mp4AtomPtr GetAvcCAtom();
	LPCSTR GetCodecName();
	LPCSTR GetType();
	UINT  GetAvgBitrate();
	UINT  GetChunkCount();
	UINT  GetMaxBitrate();
	UINT  GetMaxSampleSize();
	LPCSTR GetMediaType();
	INT64 GetMediaDuration();
	BYTE* GetParamSets(int type, int index, WORD& length );
	UINT  GetSampleCount();
	UINT  GetSyncSample(UINT index);
	UINT  GetTimeScale();
	INT64 GetTrackDuration();
	UINT  GetTrackId();
	UINT  GetVideoHeight();
	UINT  GetVideoWidth();
	UINT  GetFrameRate();
	UINT  GetChannels();

	BOOL  IsSyncSample(UINT sampleId);

	void  SetMediaDuration(INT64 duration);
	void  SetTimeScale(UINT timeScale);
	void  SetTrackAtom(Mp4Atom* track);
	void  SetTrackDuration( INT64 duration, UINT timeScale = 1000 );
	void  SetTrackId(UINT id);
	void  SetType(LPCSTR type);

// Operations -------------------------------------------------
public:
	void  AddChunk( UINT firstSampleId, UINT sampleCount, INT64 chunkOffset );
	void  AddSample( BOOL isSyncSample, UINT sampleSize, UINT duration );
	void  Clear();
	void  Init();
	void  Finish();

	UINT  GetSampleIdByOffset(INT64 fileOffset);
	UINT  GetSampleIdByTime  (INT64 when, BOOL wantSyncSample = false);
	INT64 GetSampleOffset( UINT sampleId );
	UINT  GetSampleSize  ( UINT sampleId );
	int	  GetSampleTimes ( UINT sampleId, INT64* startTime, INT64* duration );
	int	  UpdateChunkOffset(UINT offset);

// Implementation ---------------------------------------------
protected:
	void  Reset();

// Data Members -----------------------------------------------
private:
	Mp4AtomPtr		fTrackAtom;		///< 这个 Track 的根 ATOM
	Mp4PropertyPtr	fTimeScale;		///< 这个 Track 的 Time scale
	Mp4SampleTable	fSampleTable;	///< 这个 Track 的 Sample Table
};

/** Mp4Track 智能指针类型. */
typedef SmartPtr<Mp4Track> Mp4TrackPtr;

/** Mp4Track 智能指针类型数组. */
typedef Mp4Array<Mp4TrackPtr> Mp4TrackArray;

};

#endif // !defined(_NS_VISION_MP4_TRACK_H)
