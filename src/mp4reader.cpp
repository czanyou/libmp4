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
#include "mp4.h"
#include "mp4reader.h"
#include "mp4config.h"
#include "mp4file.h"
#include "mp4avcc.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Reader

/** 构建一个新的 Mp4Reader 对象. */
Mp4Reader::Mp4Reader()
{
	fAudioBuffer		= NULL;
	fAudioBufferMaxSize	= 0;
	fAudioSampleOffset	= 0;
	fAudioSampleId		= 1;
	fAudioTrackIndex	= -1;

	fNextSample			= NULL;

	fVideoBuffer		= NULL;
	fVideoBufferMaxSize	= 0;
	fVideoSampleOffset	= 0;
	fVideoSampleId		= 1;
	fVideoTrackIndex	= -1;

	fPosition			= 0;
}

/** 析构方法. */
Mp4Reader::~Mp4Reader()
{
	Clear();
}

INT64 Mp4Reader::GetSampleTime(INT64 timestamp, UINT timeScale)
{
	if (timeScale > 0 && timeScale != 1000) {
		timestamp = timestamp * 1000 / timeScale; // 换算成毫秒
	}

	return timestamp;
}

BOOL Mp4Reader::Advance()
{
	fNextSample = NULL;

	if (fVideoSampleOffset <= 0) {
		MediaFileSample* sample = GetVideoSample(fVideoSampleId);
		if (sample) {
			fVideoSampleId++;
		}
	}

	if (fAudioSampleOffset <= 0) {
		MediaFileSample* sample = GetAudioSample(fAudioSampleId);
		if (sample) {
			fAudioSampleId++;
		}
	}

	if (fAudioSampleOffset <= 0) {
		if (fVideoSampleOffset > 0) {
			fNextSample = &fVideoSample;
			fVideoSampleOffset = 0;
			return TRUE;
		}

		return FALSE;
	}

	if (fVideoSampleOffset <= 0) {
		fNextSample = &fAudioSample;
		fAudioSampleOffset = 0;
		return TRUE;
	}

	INT64 time1 = GetSampleTime(fAudioSample.GetTimestamp(), fAudioSample.GetTimeScale());
	INT64 time2 = GetSampleTime(fVideoSample.GetTimestamp(), fVideoSample.GetTimeScale());

	time2 += 2000;

	if (time1 < time2) {
		fNextSample = &fAudioSample;
		fAudioSampleOffset = 0;

	} else if (fVideoSampleOffset > 0) {
		fNextSample = &fVideoSample;
		fVideoSampleOffset = 0;
	}

	return TRUE;
}

void Mp4Reader::Clear()
{
	// 清除所有的 Track
	for (UINT i = 0; i < fTracks.GetCount(); i++) {
		Mp4TrackPtr track = fTracks[i];
		if (track) {
			track->Clear();
		}
	}

	fTracks.Clear();
	fMp4Stream.Clear();

	// 释放相关的缓存区
	if (fVideoBuffer) {
		Mp4Free(fVideoBuffer);
		fVideoBuffer = NULL;
		fVideoBufferMaxSize = 0;
		fVideoSampleOffset	= 0;
	}

	if (fAudioBuffer) {
		Mp4Free(fAudioBuffer);
		fAudioBuffer = NULL;
		fAudioBufferMaxSize = 0;
		fAudioSampleOffset	= 0;
	}

	fNextSample = NULL;
}

/** 关闭这个文件, 并释放所有的资源. */
void Mp4Reader::Close()
{
	fMp4Stream.Close();
	Clear();
}

MediaFileSample* Mp4Reader::GetAudioSample( UINT sampleId )
{
	Mp4TrackPtr track = GetTrack(fAudioTrackIndex);
	if (track == NULL) {
		return NULL;
	}

	UINT sampleSize = track->GetSampleSize(fAudioSampleId);
	if (sampleSize <= 0) {
		return NULL;
	}

	if (fAudioBuffer == NULL) {
		fAudioBuffer = (BYTE*)Mp4Malloc(1024 * 16);
		fAudioBufferMaxSize	= 1024 * 16;

	} else if (fAudioBufferMaxSize < sampleSize) {
		fAudioBuffer = (BYTE*)Mp4Realloc(fAudioBuffer, sampleSize);
		fAudioBufferMaxSize	= sampleSize;
	}

	fAudioSample.Clear();
	fAudioSample.SetData(fAudioBuffer);
	fAudioSample.SetSize(0);
	fAudioSample.SetMaxSize(fAudioBufferMaxSize);

	fAudioSampleOffset = 0;
	ReadSample(track, fAudioSampleId, &fAudioSample, &fAudioSampleOffset);

	fAudioSample.SetTrackIndex(fAudioTrackIndex);
	fAudioSample.SetSampleId(fAudioSampleId);

	return &fAudioSample;
}

/** 返回文件的创建时间, 1970-1-1 以来经过的秒数. */
INT64 Mp4Reader::GetCreationTime()
{
	return fMp4Stream.ReadCreationTime();
}

/** 返回指定的 track 的长度, 单位为毫秒. */
INT64 Mp4Reader::GetDuration()
{
	return fMp4Stream.ReadDuration();
}

Mp4FilePtr Mp4Reader::GetFile()
{
	return fMp4Stream.GetFile();
}

MediaFileSample* Mp4Reader::GetNextGpsSample()
{
	Mp4AtomPtr atom = fMp4Stream.GetGpsTrackAtom();
	if (atom == NULL) {
		return NULL;
	}

	Mp4PropertyPtr property = atom->GetProperty(0);
	if (property == NULL || property->GetType() != PT_STRING) {
		return NULL;
	}

	fTraceSample.SetData((BYTE*)property->GetBytes());
	fTraceSample.SetSize(property->GetSize());
	return &fTraceSample;
}

MediaFileSample* Mp4Reader::GetNextSyncAudioSample()
{
	Mp4TrackPtr audioTrack = GetTrack(fAudioTrackIndex);
	if (audioTrack == NULL) {
		return NULL;
	}

	UINT timeScale = audioTrack->GetTimeScale();
	INT64 sampleTime = Mp4Utils::Mp4TimeScale(fPosition, 1000, timeScale);
	fAudioSampleId = audioTrack->GetSampleIdByTime(sampleTime, TRUE);

	MediaFileSample* sample = GetAudioSample(fAudioSampleId);
	if (sample) {
		fAudioSampleId++;
	}

	return sample;
}

INT64 Mp4Reader::GetPosition()
{
	return fPosition;
}

MediaFileSample* Mp4Reader::GetSample()
{
	return fNextSample;
}

/**
 * 返回当前的 Sample ID
 * @param isVideo 
 * @return UINT 
 */
UINT Mp4Reader::GetSampleId( int trackType )
{
	return trackType ? fVideoSampleId : fAudioSampleId;
}

/** 返回指定的 track 的帧数. */
UINT Mp4Reader::GetSampleCount( int trackType )
{
	Mp4TrackPtr track = GetTrack(trackType ? fVideoTrackIndex : fAudioTrackIndex);
	return track ? track->GetSampleCount() : 0;
}

Mp4Stream& Mp4Reader::GetStream()
{
	return fMp4Stream;
}

/** 返回文件的时间比. */
UINT Mp4Reader::GetTimeScale()
{
	return fMp4Stream.ReadTimeScale();
}

/** 
 * 返回已经加载的数据的时间长度. 
 *
 * @param bytesLoaded 这个文件已经加载的字节数.
 * @return 返回这个文件已经加载的字节数对应的时间长度, 单位为毫秒.
 */
INT64 Mp4Reader::GetTimeLoaded(INT64 bytesLoaded)
{
	Mp4TrackPtr track = GetTrack(fVideoTrackIndex);
	if (track == NULL) {
		return 0;
	}

	// 取得最后的加载的一个 sample 的 ID.
	UINT sampleId = track->GetSampleIdByOffset(bytesLoaded);
	if (sampleId <= 0) {
		return 0;
	}

	// 返回这个 sample 对应的时间戳信息
	INT64 startTime = 0;
	INT64 duration  = 0;
	if (track->GetSampleTimes(sampleId, &startTime, &duration) != 0) {
		return 0;
	}

	// 换算成毫秒
	return Mp4Utils::Mp4TimeScale(startTime, track->GetTimeScale(), 1000);
}

MediaFileFormatPtr Mp4Reader::GetTrackFormat( int trackIndex )
{
	return (MediaFileFormat*)GetTrack(trackIndex).ptr;
}

/** 返回指定的索引的 track. */
Mp4TrackPtr Mp4Reader::GetTrack( UINT trackIndex )
{
	if (trackIndex >= fTracks.GetCount()) {
		return NULL;
	}	

	return fTracks[trackIndex];
}

/** 返回指定的类型的 track. */
Mp4TrackPtr Mp4Reader::GetTrack( LPCSTR type )
{
	for (UINT i = 0; i < fTracks.GetCount(); i++) {
		Mp4TrackPtr track = fTracks[i];
		LPCSTR trackType = track->GetType();
		if (track && trackType && !strcmp(trackType, type)) {
			return track;
		}
	}

	return NULL;
}

/** 返回这个 Mp4 文件的 Track 的数目. */
UINT Mp4Reader::GetTrackCount()
{
	return fTracks.GetCount();
}

/** 返回下一个视频 sample 的内容. */
MediaFileSample* Mp4Reader::GetVideoSample(UINT sampleId)
{
	Mp4TrackPtr videoTrack = GetTrack(fVideoTrackIndex);
	if (videoTrack == NULL) {
		return NULL;
	}

	// 返回指定的 ID 的 Sample 的长度.
	UINT sampleSize = videoTrack->GetSampleSize(sampleId);
	if (sampleSize <= 0 || sampleSize >= 1024 * 1024) {
		return NULL;
	}

	// 分配足够的缓存区空间
	if (fVideoBuffer == NULL) {
		sampleSize	 = (sampleSize > 1024 * 256) ? sampleSize : 1024 * 256;
		fVideoBuffer = (BYTE*)Mp4Malloc(sampleSize);
		fVideoBufferMaxSize	= sampleSize;

	} else if (fVideoBufferMaxSize < sampleSize) {
		fVideoBuffer = (BYTE*)Mp4Realloc(fVideoBuffer, sampleSize);
		fVideoBufferMaxSize	= sampleSize;
	}

	// 重置相应的属性
	fVideoSample.Clear();
	fVideoSample.SetData(fVideoBuffer);
	fVideoSample.SetSize(0);
	fVideoSample.SetMaxSize(fVideoBufferMaxSize);

	// 读取这个 sample 的内容和属性. 
	fVideoSampleOffset = 0;
	int ret = ReadSample(videoTrack, sampleId, &fVideoSample, &fVideoSampleOffset);
	if (ret <= 0 || ret != (int)fVideoSample.GetSize()) {
		return NULL;
	}

	fVideoSample.SetTrackIndex(fVideoTrackIndex);
	fVideoSample.SetSampleId(sampleId);

	// 更新当前播放位置, 单位为 MS.
	INT64 timestamp = fVideoSample.GetTimestamp();
	fPosition = Mp4Utils::Mp4TimeScale(timestamp, videoTrack->GetTimeScale(), 1000);

	return &fVideoSample;
}

/** 指出文件是否已经结束. */
BOOL Mp4Reader::IsEndOfFile()
{
	Mp4TrackPtr track = GetTrack(fVideoTrackIndex);
	if (track == NULL) {
		return TRUE;
	}

	UINT count = track->GetSampleCount();
	if (fVideoSampleId > count) {
		return TRUE;
	}

	return FALSE;
}

int Mp4Reader::LoadMoovAtom()
{
	if (GetTrackCount() > 0) {
		return 0;
	}

	int ret = fMp4Stream.LoadMoovAtom();
	if (ret < 0) {
		return ret;
	}

	// 初始化相关的成员变量
	fAudioSampleId		= 1;
	fAudioTrackIndex	= -1;
	fPosition			= 0;
	fVideoSampleId		= 1;
	fVideoTrackIndex	= -1;

	// 创建相关的 track.
	LoadTracks();
	return 0;
}

int Mp4Reader::LoadMvhdAtom()
{
	int ret = fMp4Stream.LoadMvhdAtom();
	if (ret < 0) {
		return ret;
	}

	// 初始化相关的成员变量
	fAudioSampleId		= 1;
	fAudioTrackIndex	= -1;
	fPosition			= 0;
	fVideoSampleId		= 1;
	fVideoTrackIndex	= -1;

	return 0;
}

/** 生成相应的 track. */
int Mp4Reader::LoadTracks()
{
	Mp4AtomPtr moov = fMp4Stream.GetMoovAtom();
	if (moov == NULL) {
		return 0;	// 没有找到 moov 节点
	}

	UINT trakId = Mp4Utils::AtomId("trak");

	int count = moov->GetChildrenCount();
	for (int i = 0; i < count; i++) {
		Mp4AtomPtr atom = moov->GetChildAtom(i);
		if (atom == NULL) {
			break;
		}

		// 一个 trak atom 节点代表一个 track 
		if (trakId != Mp4Utils::AtomId(atom->GetType()) ) {
			continue;
		}

		Mp4TrackPtr track = new Mp4Track();
		track->SetTrackAtom(atom);

		if (Mp4Utils::AtomId(track->GetType()) == Mp4Utils::AtomId("vide")) {
			fVideoTrackIndex = fTracks.GetCount();  // 记录视频 track 的索引便于查找

		} else if (Mp4Utils::AtomId(track->GetType()) == Mp4Utils::AtomId("soun")) {
			fAudioTrackIndex = fTracks.GetCount();
		}

		fTracks.Add(track);
	}

	return fTracks.GetCount();
}

/** 
 * 打开并读取指定的 MP4 文件. 
 * @param filename 要打开的文件的文件名.
 * @return 如果成功, 则返回 MP4_S_OK.
 */
int Mp4Reader::Open( LPCSTR filename, UINT flags )
{
	if (filename == NULL) {
		return MP4_ERR_FAILED;
	} 

	Mp4FilePtr file = new Mp4File();

	int ret = file->Open(filename, "rb");
	if (ret != MP4_S_OK) {
		Close();
		return ret;
	}

	return Open(file, flags);
}

/**
 * 打开指定的 MP4 文件. 
 * @param file 
 * @return 如果成功, 则返回 MP4_S_OK.
 */
int Mp4Reader::Open( Mp4File* file, UINT flags )
{
	if (GetFile() != NULL) {
		return MP4_ERR_ALREADY_OPEN;
	}

	int ret = fMp4Stream.OpenRead(file, flags);
	if (ret < 0) {
		Close();
		return ret;
	}

	if (flags == 0) {
		LoadMoovAtom();

	} else {
		LoadMvhdAtom();
	}

	return ret;
}

/**
 * 读取指定的 sample 的内容.
 * @param file 要读取的文件
 * @param sampleId 要读取的 Sample 的 ID
 * @param sample 输出参数, 保存读取的 Sample 的内容和属性.
 * @return 如果成功则返回 0. 
 */
int Mp4Reader::ReadSample( Mp4TrackPtr track, UINT sampleId, 
						  MediaFileSample* sample, INT64* offset)
{
	Mp4FilePtr file = GetFile();
	if (track == NULL || sampleId == 0 || file == NULL || sample == NULL) {
		return MP4_ERR_FAILED;
	}

	// Sample size
	UINT ret = 0;
	UINT sampleSize = track->GetSampleSize(sampleId);			// 长度
	if (sampleSize <= 0) {
		sample->SetSize(0);
		return ret;
	}

	// Sample Data
	BYTE* data    = sample->GetData();
	UINT maxSize  = sample->GetMaxSize();
	sample->SetSize(sampleSize);

	if (data && maxSize > 0) {
		INT64 fileOffset = track->GetSampleOffset(sampleId);	// 偏移位置
		if (file->SetPosition(fileOffset) == 0) {
			UINT copySize = (sampleSize > maxSize) ? maxSize : sampleSize;
			ret = file->ReadBytes(data, copySize);
		}

		if (offset) {
			*offset = fileOffset;
		}
	}

	// Sample attributes
	INT64 timestamp = 0;
	INT64 duration  = 0;
	track->GetSampleTimes(sampleId, &timestamp, &duration);

	sample->SetTimestamp(timestamp);
	sample->SetDuration(duration);
	sample->SetTimeScale(track->GetTimeScale());
	sample->SetSyncPoint(track->IsSyncSample(sampleId));

	return ret;
}

/** 
 * 设置当前播放位置. 
 * @param position 新的播放位置. 
 */
void Mp4Reader::SeekTo( INT64 position )
{
	Mp4TrackPtr track = GetTrack(fVideoTrackIndex);
	if (track) {
		UINT timeScale = track->GetTimeScale();
		INT64 pts = Mp4Utils::Mp4TimeScale(position, 1000, timeScale);
		fVideoSampleId = track->GetSampleIdByTime(pts, TRUE);

		// 取得当前视频帧的时间戳
		if (fVideoSampleId > 0) {
			INT64 startTime = 0, duration = 0;
			track->GetSampleTimes(fVideoSampleId, &startTime, &duration);
			fPosition = Mp4Utils::Mp4TimeScale(startTime, timeScale, 1000);

		} else {
			fPosition = position;
		}
	}

	// 重位定位音频 TRACK 的位置
	track = GetTrack(fAudioTrackIndex);
	if (track) {
		INT64 pts = Mp4Utils::Mp4TimeScale(fPosition, 1000, track->GetTimeScale());
		fAudioSampleId = track->GetSampleIdByTime(pts, TRUE);
	}

	fAudioSampleOffset = 0;
	fVideoSampleOffset = 0;
	fNextSample = NULL;
}

/** 
 * 移动 MP4 文件头 (moov Atom) 的位置到文件到开始位置, 并删除空闲的
 * 头部数据. 这样做的目的主要是为了方便边下载边播放.
 *
 */
void Mp4Reader::UpdateChunksOffset()
{
	INT64 offset = fMp4Stream.GetChunksOffset();
	if (offset == 0) {
		return;
	}

	// 更新所有的 Chunk 的偏移位置
	for (UINT i = 0; i < fTracks.GetCount(); i++) {
		Mp4TrackPtr track = fTracks[i];
		track->UpdateChunkOffset((UINT)offset);
	}
}

};
