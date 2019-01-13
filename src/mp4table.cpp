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
#include "mp4table.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4SampleTable

Mp4SampleTable::Mp4SampleTable()
{
	fFixedSampleSize	= 0;
	fChunkCount			= 0;
}

/** 添加一个新的 Chunk. */
void Mp4SampleTable::AddChunk( UINT sampleId, UINT samplesPerChunk, INT64 chunkOffset )
{
	fChunkCount++;
	AddChunkToSample(sampleId, fChunkCount, samplesPerChunk);
	AddChunkOffset(chunkOffset);
}

/**
 * 更新 Chunk Offset Table.
 * Chunk Offset Table 很简单, 只有一列, 记录每一个 Chunk 的偏移位置.
 * @param chunkOffset 新添加的 Chunk 在文件中的偏移位置.
 */
void Mp4SampleTable::AddChunkOffset( INT64 chunkOffset )
{
	// TODO: 64 位偏移
	if (fChunksOffsets != NULL) {
		fChunksOffsets->AddRow((UINT)chunkOffset);
	}
}

/**
 * 更新 Sample-To-Chunk 表. 
 * Sample-To-Chunk 表有三列, 分别是:
 * - First Chunk: 使用这个表格实体的第一个 Chunk 的 ID.
 * - Sample per chunk: 每一个 Chunk 包含的 Sample 的数量.
 * - Sample description ID: 
 * @param sampleId 当前 Sample 的 ID.
 * @param chunkId 当前 Chunk 的 ID.
 * @param samplesPerChunk 每一个 Chunk 包含的 Sample 的数量.
 */
void Mp4SampleTable::AddChunkToSample( UINT sampleId, UINT chunkId, UINT samplesPerChunk )
{
	if (fSampleToChunk == NULL || fFirstSampleToChunk == NULL) {
		return;
	}

	UINT numStsc = fSampleToChunk->GetRowCount();	
	UINT lastSamplesPerChunk = fSampleToChunk->GetValue(numStsc - 1, 1);
	// if (samplesPerChunk) == (samplesPerChunk of last entry)	
	if (numStsc && (samplesPerChunk == lastSamplesPerChunk)) {
		return;
	}

	// add stsc entry
	fSampleToChunk->AddRow(chunkId, samplesPerChunk, 1);
	fFirstSampleToChunk->AddRow(sampleId);	
}

/** 添加一个新的 Sample. */
void Mp4SampleTable::AddSample( BOOL isSyncPoint, UINT size, INT64 duration )
{
	UINT sampleId = GetSampleCount() + 1;

	// 第一个 sample 的时间长度通常为 0, 下一个 sample 的时间才是前一个 sample 
	// 真正经过的时间, 所以跳过第一个 sample 传入的时间, 并在写完后, 要补一个 
	// sample 的时间, 见 FinishWrite
	if (sampleId > 1) {
		AddSampleTime(duration);
	}

	AddSampleSize(size);
	if (isSyncPoint) {
		AddSyncSample(sampleId);
	}
}

/**
 * 更新 Sample Size (stsz) 表, 添加一个 Sample 大小项.
 * stsz 只包含一列, 就是每个 Sample 的大小 (Size), 32 位整数, 单位为字节.
 *
 * Sample Size Atom 有一个全局的属性: "Fixed Sample Size", 当这个属性为 
 * 0 时, 表示每个 Sample 会有不同的大小, 否则所有的 Sample 都是同样
 * 的大小.
 *
 * @param sampleId 这个 Sample 的 ID.
 * @param numBytes 这个 Sample 的大小.
 */
void Mp4SampleTable::AddSampleSize( UINT sampleSize )
{
	if (fSampleSize) {
		fSampleSize->AddRow(sampleSize);
	}
}

/**
 * 更新 Time-To-Sample Table (stts).
 * stts 包含两列, Sample Count 和 Sample Delta. Sample Count 表示连续的
 * 有相同时间长度的 Sample 的个数, Sample Delta 表示每个 Sample 的时间长度, 
 *
 * @param duration 新添加的 Sample 的时间长度.
 */
void Mp4SampleTable::AddSampleTime( INT64 duration )
{
	if (fSampleTimes == NULL) {
		return;
	}

	UINT numStts = fSampleTimes->GetRowCount();

	// if duration == duration of last entry
	if (numStts > 0 && duration == fSampleTimes->GetValue(numStts - 1, 1)) {
		// increment last entry sampleCount
		UINT index = numStts - 1;
		fSampleTimes->SetValue(index, fSampleTimes->GetValue(index) + 1);

	} else {
		// add stts entry, sampleCount = 1, sampleDuration = duration
		fSampleTimes->AddRow(1, (UINT)duration);
	}
}

/**
 * 更新 Sync Sample (stss) 表.
 * stss 只有一列, 包含了所有同步 Sample 的 ID.
 * @param sampleId Sample 的 ID.
 * @param isSyncSample 指出是否是同步 Sample.
 */
void Mp4SampleTable::AddSyncSample( UINT sampleId )
{
	// if stss atom exists, add entry
	if (fSampleSyncPoints) {
		fSampleSyncPoints->AddRow(sampleId);
	}
}

void Mp4SampleTable::Clear()
{
	fChunksOffsets		= NULL;
	fFirstSampleToChunk	= NULL;
	fSampleSize			= NULL;
	fSampleSyncPoints	= NULL;
	fSampleTimes		= NULL;
	fSampleTimestamps	= NULL;
	fSampleToChunk		= NULL;
	fTrackAtom			= NULL;
}

int Mp4SampleTable::Finish()
{
	if (fTrackAtom == NULL) {
		return MP4_ERR_FAILED;

	} else if (fSampleTimes == NULL || fChunksOffsets == NULL) {
		return MP4_ERR_FAILED;

	} else if (fSampleToChunk == NULL || fSampleSize == NULL) {
		return MP4_ERR_FAILED;
	}

	Mp4AtomPtr stbl = fTrackAtom->FindAtom("mdia.minf.stbl");
	if (stbl == NULL) {
		return MP4_ERR_FAILED;
	}

	if (fSampleSize->GetRowCount() == 0) {
		stbl->SetPropertyInt("stsz.sampleSize", fFixedSampleSize);
		stbl->SetPropertyInt("stsz.entryCount", 0);

	} else {
		stbl->SetPropertyInt("stsz.sampleSize", 0);
		stbl->SetPropertyInt("stsz.entryCount", fSampleSize->GetRowCount());
	}

	stbl->SetPropertyInt("stsc.entryCount", fSampleToChunk->GetRowCount());
	stbl->SetPropertyInt("stco.entryCount", fChunksOffsets->GetRowCount());
	stbl->SetPropertyInt("stts.entryCount", fSampleTimes->GetRowCount());

	if (fSampleSyncPoints != NULL) {
		stbl->SetPropertyInt("stss.entryCount", fSampleSyncPoints->GetRowCount());
	}
	return MP4_S_OK;
}

/** 返回包含的 Chunk 的数量. */
UINT Mp4SampleTable::GetChunkCount()
{
	return fChunksOffsets ? fChunksOffsets->GetRowCount() : fChunkCount;
}

/**
 * 返回指定的文件偏移位置所属的 Chunk 的 ID.
 *
 * @param fileOffset 文件偏移位置, 单位为字节.
 * @return 如果在存则返回这个 Chunk 的 ID, 否则返回 0.
 */
UINT Mp4SampleTable::GetChunkIdByOffset(INT64 fileOffset)
{
	if (fChunksOffsets == NULL) {
		return 0;
	}

	UINT sid = fChunksOffsets->Search((UINT)fileOffset);
	if (sid == 0) {
		UINT offset = fChunksOffsets->GetValue(sid);
		if (fileOffset < offset) {
			return 0;
		}
	}

	return sid + 1;
}

/** 返回属于指定的 Chunk 的第一个 Sample 的 ID. */
UINT Mp4SampleTable::GetFirstSampleOfChunk(UINT chunkId)
{
	if (chunkId <= 1) {
		return 1;
	}

	UINT firstSampleId = 1;
	if (fFirstSampleToChunk && fSampleToChunk) {
		UINT stscIndex = fSampleToChunk->Search(chunkId);
		firstSampleId  = fFirstSampleToChunk->GetValue(stscIndex);

		UINT firstChunk		 = fSampleToChunk->GetValue(stscIndex, 0);
		UINT samplesPerChunk = fSampleToChunk->GetValue(stscIndex, 1);
		firstSampleId += (chunkId - firstChunk) * samplesPerChunk;
	}

	return (firstSampleId <= 0) ? 1 : firstSampleId;
}

/** 返回包含的 Sample 的数量. */
UINT Mp4SampleTable::GetSampleCount()
{
	if (fSampleSize != NULL) {
		return fSampleSize->GetRowCount();
	}

	return fSampleTimes ? fSampleTimes->GetRowCount() : 0;
}

/**
 * 返回指定的文件偏移位置所在的 Sample 的 ID.
 *
 * @param fileOffset 文件偏移位置, 单位为字节.
 * @return 如果在存则返回这个 Sample 的 ID, 否则返回 0.
 */
UINT Mp4SampleTable::GetSampleIdByOffset(INT64 fileOffset)
{
	UINT chunkId = GetChunkIdByOffset(fileOffset);
	if (chunkId <= 0) {
		return 0;
	}

	UINT firstSampleId = GetFirstSampleOfChunk(chunkId);
	UINT count = GetSampleCount();
	UINT sampleId = firstSampleId - 1;
	for (; sampleId < count; sampleId++) {
		UINT offset = (UINT)GetSampleOffset(sampleId + 1);
		if (fileOffset < offset) {
			break;
		}
	}

	return sampleId;
}

/**
 * 返回指定的时间对应的 sample 的 ID. 
 *
 * @param when 文件相对时间.
 * @param wantSyncSample 是否希望返回同步 Sample 的 ID.
 * @return 返回找到的 Sample 的 ID. 
 */
UINT Mp4SampleTable::GetSampleIdByTime( INT64 when, BOOL wantSyncSample /*= false*/ )
{
	if (fSampleTimes == NULL) {
		return 0;
	}

	UINT numStts = fSampleTimes->GetRowCount();
	UINT sid = 1;
	INT64 elapsed = 0;

	if (fSampleTimestamps) {
		UINT row = fSampleTimestamps->Search((UINT)when, 1);
		sid		 = fSampleTimestamps->GetValue(row, 0);
		elapsed  = fSampleTimestamps->GetValue(row, 1);
		UINT sampleDelta = fSampleTimes->GetValue(row, 1);

		INT64 d = when - elapsed;
		UINT sampleId = sid;
		if (sampleDelta) {
			sampleId += (UINT)(d / sampleDelta);
		}

		if (wantSyncSample) {
			UINT syncSampleId = GetPrevSyncSample(sampleId);
			return (syncSampleId > 0) ? syncSampleId : sampleId;
		}
		return sampleId;
	}

	for (UINT sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
		UINT sampleCount = fSampleTimes->GetValue(sttsIndex, 0);
		UINT sampleDelta = fSampleTimes->GetValue(sttsIndex, 1);

		INT64 d = when - elapsed;
		if (d <= sampleCount * sampleDelta) {
			UINT sampleId = sid;
			if (sampleDelta) {
				sampleId += (UINT)(d / sampleDelta);
			}

			if (wantSyncSample) {
				UINT syncSampleId = GetPrevSyncSample(sampleId);
				return (syncSampleId > 0) ? syncSampleId : sampleId;
			}
			return sampleId;
		}

		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;
	}

	return 0; // satisfy MS compiler
}

/**
 * 返回指定的 ID 的 sample 在文件中的偏移位置.
 *
 * @param sampleId Sample 的 ID.
 * @return 偏移位置.
 */
INT64 Mp4SampleTable::GetSampleOffset( UINT sampleId )
{
	if (fSampleToChunk == NULL || fFirstSampleToChunk == NULL 
	 || fChunksOffsets  == NULL) {
		return 0;
	}

	UINT row = fFirstSampleToChunk->Search(sampleId);

	// firstChunk is the chunk index of the first chunk with 
	// samplesPerChunk samples in the chunk.  There may be multiples -
	// ie: several chunks with the same number of samples per chunk.
	UINT firstChunk		 = fSampleToChunk->GetValue(row, 0);
	UINT samplesPerChunk = fSampleToChunk->GetValue(row, 1);
	UINT firstSample	 = fFirstSampleToChunk ->GetValue(row);
	if (samplesPerChunk == 0) {
		return 0;
	}
	
	// chunkId tells which is the absolute chunk number that this sample
	// is stored in.
	UINT chunkId = firstChunk +	((sampleId - firstSample) / samplesPerChunk);
	
	// chunkOffset is the file offset (absolute) for the start of the chunk
	INT64 chunkOffset = fChunksOffsets->GetValue(chunkId - 1);	

	// 
	UINT firstSampleInChunk = sampleId - ((sampleId - firstSample) % samplesPerChunk);
	
	// need cumulative samples sizes from firstSample to sampleId - 1
	UINT sampleOffset = 0;
	for (UINT i = firstSampleInChunk; i < sampleId; i++) {
		sampleOffset += GetSampleSize(i);
	}
	
	return chunkOffset + sampleOffset;	
}

/** 返回最大的 Sample 的大小. */
UINT Mp4SampleTable::GetSampleMaxSize()
{
	if (fSampleSize == NULL) {
		return fFixedSampleSize;
	}
	
	UINT sampleCount = fSampleSize->GetRowCount();
	if (sampleCount <= 0) {
		return fFixedSampleSize;
	}

	UINT maxSampleSize = 0;
	for (UINT index = 0; index < sampleCount; index++) {
		UINT sampleSize = fSampleSize->GetValue(index);
		if (sampleSize > maxSampleSize) {
			maxSampleSize = sampleSize;
		}
	}
	return maxSampleSize;
}

/**
 * 返回指定的 ID 的 Sample 的大小.
 *
 * @param sampleId 这个 Sample 的 ID.
 * @return Sample 的大小. 
 */
UINT Mp4SampleTable::GetSampleSize( UINT sampleId )
{
	if (sampleId <= 0) {
		return 0;

	} else if (fSampleSize && fSampleSize->GetRowCount() > 0) {
		return fSampleSize->GetValue(sampleId - 1);

	} else {
		return fFixedSampleSize;
	}
}

/** 返回所有的 Sample 总共的大小. */
INT64 Mp4SampleTable::GetSampleTotalSize()
{
	INT64 retval = 0;
	
	if (fSampleSize && fSampleSize->GetRowCount() > 0) {
		// non-fixed sample size, sum them
		UINT sampleCount = fSampleSize->GetRowCount();
		for (UINT index = 0; index < sampleCount; index++) {
			retval += fSampleSize->GetValue(index);
		}

	} else {
		// if fixed sample size, just need to multiply by number of samples
		retval = fFixedSampleSize * GetSampleCount();
	}

	return retval;
}

/**
 * 返回指定的 ID 的 Sample 的时间信息.
 *
 * @param sampleId 这个 Sample 的 ID.
 * @param pStartTime 输出参数, 返回这个 Sample 的开始时间.
 * @param pDuration 输出参数, 返回这个 Sample 的持续时间.
 * @return 如果成功则返回 0, 否则返回 -1. 
 */
int Mp4SampleTable::GetSampleTimes( UINT sampleId, INT64* pStartTime, INT64* pDuration )
{
	if (fSampleTimes == NULL) {
		return -1;
	}
	
	UINT numStts = fSampleTimes->GetRowCount();
	UINT sid = 1;
	INT64 elapsed = 0;

	if (fSampleTimestamps) {
		//LOG_D("sampleId: %d\r\n", sampleId);

		UINT row = fSampleTimestamps->Search(sampleId);
		//LOG_D("row: %d\r\n", row);

		sid		 = fSampleTimestamps->GetValue(row, 0);
		//LOG_D("sid: %d\r\n", sid);

		elapsed  = fSampleTimestamps->GetValue(row, 1);
		//LOG_D("elapsed: %d\r\n", elapsed);

		UINT sampleDelta = fSampleTimes->GetValue(row, 1);
		//LOG_D("sampleDelta: %d\r\n", sampleDelta);

		if (pStartTime) {
			*pStartTime = (sampleId - sid);
			*pStartTime *= sampleDelta;
			*pStartTime += elapsed;
		}

		if (pDuration) {
			*pDuration = sampleDelta;
		}
		
		return 0;

	} else {
		for (UINT sttsIndex = 0; sttsIndex < numStts; sttsIndex++) {
			UINT sampleCount = fSampleTimes->GetValue(sttsIndex, 0);
			UINT sampleDelta = fSampleTimes->GetValue(sttsIndex, 1);
			
			if (sampleId <= sid + sampleCount - 1) {
				if (pStartTime) {
					*pStartTime = (sampleId - sid);
					*pStartTime *= sampleDelta;
					*pStartTime += elapsed;
				}

				if (pDuration) {
					*pDuration = sampleDelta;
				}
				return 0;
			}
			sid += sampleCount;
			elapsed += sampleCount * sampleDelta;
		}
	}
	return -1;
}

/**
 * 返回指定的 sampleId 之后的下一个关键帧.
 * @param sampleId 参考 Sample 的 ID. 
 * @return 返回相应的关键帧的 ID. 
 */
UINT Mp4SampleTable::GetNextSyncSample(UINT sampleId)
{
	if (fSampleSyncPoints == NULL) {
		return sampleId; // 如果不存在 stss 则每一帧都是关键帧
	}

	if (sampleId <= 0) {
		return GetSyncSample(0);
	}
	
	UINT row = fSampleSyncPoints->Search(sampleId);

	UINT syncSampleId = fSampleSyncPoints->GetValue(row);
	if (syncSampleId == sampleId) {
		return sampleId;
	}

	return GetSyncSample(row + 1);
}

/**
 * 返回指定的 ID 的 sample 之前的最近的一个关键帧.
 *
 * @param sampleId 参考 Sample 的 ID.
 * @return 返回相应的关键帧的 ID. 
 */
UINT Mp4SampleTable::GetPrevSyncSample(UINT sampleId)
{
	if (fSampleSyncPoints == NULL) {
		return sampleId; // 如果不存在 stss 则每一帧都是关键帧

	} else if (sampleId <= 0) {
		return 0;
	}
	
	UINT row = fSampleSyncPoints->Search(sampleId);
	return fSampleSyncPoints->GetValue(row);
}

/**
 * 返回指定的索引的同步 Sample 的 ID.
 *
 * @param index 索引位置.
 * @return 对应的同步 Sample 的 ID.
 */
UINT Mp4SampleTable::GetSyncSample(UINT index)
{
	if (fSampleSyncPoints && index < fSampleSyncPoints->GetRowCount()) {
		return fSampleSyncPoints->GetValue(index);
	}
	return 0;
}

/**
 * 指出指定的 ID 的 Sample 是否是同步 Sample. 
 * @param sampleId Sample 的 ID.
 * @return 如果是则返回 TRUE, 否则返回 FALSE. 
 */
BOOL Mp4SampleTable::IsSyncSample( UINT sampleId )
{
	if (fSampleSyncPoints == NULL) {
		return TRUE; // 如果不存在 stss 则认为每一帧都是关键帧
	}

	UINT row = fSampleSyncPoints->Search(sampleId);
	return fSampleSyncPoints->GetValue(row) == sampleId;
}

void Mp4SampleTable::InitFirstSampleToChunkTable()
{
	// 构建一个 firstSample 以便于查询
	if (fSampleToChunk == NULL) {
		return;
	}

	UINT sampleId = 1;
	fFirstSampleToChunk = new Mp4TableProperty("firstSample");
	fFirstSampleToChunk->AddColumn("firstSample");

	UINT count = fSampleToChunk->GetRowCount();
	for (UINT i = 0; i < count; i++) {
		fFirstSampleToChunk->AddRow(sampleId);

		if (i < count - 1) {
			UINT nextChunkId	= fSampleToChunk->GetValue(i + 1, 0);
			UINT chunkId		= fSampleToChunk->GetValue(i, 0);
			UINT samplePerChunk = fSampleToChunk->GetValue(i, 1);
			sampleId +=	(nextChunkId - chunkId) * samplePerChunk;
		}
	}
}

void Mp4SampleTable::InitSampleTimestampsTable()
{
	if (fSampleTimes == NULL) {
		return;
	}

	fSampleTimestamps = new Mp4TableProperty("SampleTimestamps");
	fSampleTimestamps->AddColumn("firsetSample");	// 第一个 Sample 的 ID
	fSampleTimestamps->AddColumn("timestamp");		// 这些 Sample 的时间

	UINT numStts = fSampleTimes->GetRowCount();
	UINT sid = 1;
	INT64 elapsed = 0;

	for (UINT row = 0; row < numStts; row++) {
		UINT sampleCount = fSampleTimes->GetValue(row, 0);
		UINT sampleDelta = fSampleTimes->GetValue(row, 1);

		fSampleTimestamps->AddRow(sid, (UINT)elapsed);

		sid += sampleCount;
		elapsed += sampleCount * sampleDelta;

		//LOG_D("%d,%d,\r\n", sampleCount, sampleDelta);
	}
}

void Mp4SampleTable::Init(Mp4Atom* track)
{
	if (track == NULL) {
		return;
	}

	fTrackAtom		= track;
	Mp4AtomPtr stbl = fTrackAtom->FindAtom("mdia.minf.stbl");
	if (stbl == NULL) {
		return;
	}

	fSampleSize	= (Mp4TableProperty*)stbl->FindProperty("stsz.entries").ptr;
	fSampleSyncPoints	= (Mp4TableProperty*)stbl->FindProperty("stss.entries").ptr;
	fChunksOffsets	= (Mp4TableProperty*)stbl->FindProperty("stco.entries").ptr;
	fSampleTimes	= (Mp4TableProperty*)stbl->FindProperty("stts.entries").ptr;
	fSampleToChunk	= (Mp4TableProperty*)stbl->FindProperty("stsc.entries").ptr;
	fChunkCount		= GetChunkCount();

	InitFirstSampleToChunkTable();
	InitSampleTimestampsTable();
}

void Mp4SampleTable::Reset()
{
	fChunkCount			= 0;
	fFixedSampleSize	= 0;
}

/**
 * 设置固定的 Sample 长度.
 * @param duration 固定的 Sample 长度.
 */
void Mp4SampleTable::SetFixedSampleDuration( INT64 duration )
{
	if (fSampleSize == NULL || fSampleSize->GetRowCount() <= 0) {
		fFixedSampleSize = (UINT)duration;
	}
}

int Mp4SampleTable::UpdateChunkOffset( int offset )
{
	if (fChunksOffsets == NULL) {
		return 0;
	}

	Mp4TablePropertyPtr chunks = fChunksOffsets;
	for (UINT j = 0; j < chunks->GetRowCount(); j++) {
		UINT lastOffset = chunks->GetValue(j);
		chunks->SetValue(j, UINT(lastOffset + offset));
	}

	return 0;
}

}
