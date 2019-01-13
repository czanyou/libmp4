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
#ifndef _NS_VISION_MP4_SAMPLE_TABLE_H
#define _NS_VISION_MP4_SAMPLE_TABLE_H

#include "mp4common.h"
#include "mp4atom.h"
#include "mp4.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4SampleTable

/**
 * 代表一个 Sample Table.
 * A packed directory for the timing and physical layout of the samples in a track.
 *
 * @author ChengZhen (anyou@msn.com)
 */ 
class Mp4SampleTable
{
public:
	Mp4SampleTable();

	typedef Mp4Array<INT64> INT64Array;

// Attributes -------------------------------------------------
public:
	UINT  GetChunkCount();
	UINT  GetChunkIdByOffset (INT64 fileOffset);
	UINT  GetFirstSampleOfChunk(UINT chunkId);
	UINT  GetNextSyncSample(UINT sampleId);
	UINT  GetPrevSyncSample(UINT sampleId);

	UINT  GetSampleCount();
	UINT  GetSampleIdByOffset(INT64 fileOffset);
	UINT  GetSampleIdByTime( INT64 when, BOOL wantSyncSample /*= false*/ );
	UINT  GetSampleMaxSize();
	INT64 GetSampleOffset( UINT sampleId );
	UINT  GetSampleSize( UINT sampleId );
	int   GetSampleTimes( UINT sampleId, INT64* pStartTime, INT64* pDuration );
	INT64 GetSampleTotalSize();
	UINT  GetSyncSample(UINT index);

	BOOL  IsSyncSample( UINT sampleId );
	void  SetFixedSampleDuration( INT64 duration );

// Operations -------------------------------------------------
public:
	void  AddSample(BOOL isSyncPoint, UINT sampleSize, INT64 duration);
	void  AddSampleSize(UINT sampleSize);
	void  AddSampleTime(INT64 duration);
	void  AddSyncSample(UINT sampleId);

	void  AddChunk(UINT sampleId, UINT samplesPerChunk, INT64 chunkOffset);
	void  AddChunkOffset(INT64 chunkOffset);
	void  AddChunkToSample(UINT sampleId, UINT chunkId, UINT samplesPerChunk);

	void  Clear();
	int   Finish();
	void  Init(Mp4Atom* track);
	void  InitSampleTimestampsTable();
	void  InitFirstSampleToChunkTable();
	void  Reset();
	int   UpdateChunkOffset(int offset);

// Data Members -----------------------------------------------
private:
	Mp4TablePropertyPtr fChunksOffsets;			///< Chunk 文件偏移位置表
	Mp4TablePropertyPtr fFirstSampleToChunk;	///< 第一个 sample 的 ID.
	Mp4TablePropertyPtr fSampleTimes;			///< stts
	Mp4TablePropertyPtr fSampleTimestamps;		///< 
	Mp4TablePropertyPtr fSampleToChunk;			///< stsc
	Mp4TablePropertyPtr fSampleSize;			///< Sample 大小表
	Mp4TablePropertyPtr fSampleSyncPoints;		///< 同步 Sample ID 表

	Mp4AtomPtr	fTrackAtom;			///< 根 ATOM
	UINT  fChunkCount;				///< Chunk 的数量
	UINT  fFixedSampleSize;			///< 固定的 sample 的时间长度
};

}

#endif // _NS_VISION_MP4_SAMPLE_TABLE_H

