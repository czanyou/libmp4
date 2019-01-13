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
#include <math.h>
#include "mp4track.h"
#include "mp4config.h"
#include "mp4avcc.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Track class

/** 构建一个新的 Mp4Track. */
Mp4Track::Mp4Track()
{
	Reset();
}

Mp4Track::~Mp4Track()
{
}

void Mp4Track::AddChunk( UINT firstSampleId, UINT sampleCount, INT64 chunkOffset )
{
	fSampleTable.AddChunk(firstSampleId, sampleCount, chunkOffset);
}

/**
 * 写入指定的 sample.
 * @param file 
 * @param sample 
 */
void Mp4Track::AddSample( BOOL isSyncPoint, UINT sampleSize, UINT duration )
{
	fSampleTable.AddSample(isSyncPoint, sampleSize, duration);
}


/** 清除这个 Mp4Track 的内容, 重置所有的属性的值. */
void Mp4Track::Clear()
{
	if (fTrackAtom != NULL) {
		fTrackAtom->Clear();
	}

	Reset();

	fTrackAtom			= NULL;
	fTimeScale			= NULL;
}

void Mp4Track::Finish()
{
	fSampleTable.Finish();
}

/** 返回 AvcC atom 的指针. */
Mp4AtomPtr Mp4Track::GetAvcCAtom()
{
	return fTrackAtom ? fTrackAtom->FindAtom("mdia.minf.stbl.stsd.avc1.avcC") : NULL;
}

/** 返回这个 Track 的平均码率. */
UINT Mp4Track::GetAvgBitrate()
{
	INT64 duration = GetMediaDuration();
	if (duration == 0) {
		return 0;
	}

	double calc = (double)fSampleTable.GetSampleTotalSize();
	// this is a bit better - we use the whole duration
	calc *= 8.0;
	calc *= GetTimeScale();
	calc /=	duration;
	// we might want to think about rounding to the next 100 or 1000
	return (UINT)ceil(calc);
}

UINT Mp4Track::GetChannels()
{
	Mp4AtomPtr atom = GetTrackAtom();
	if (atom == NULL) {
		return 1;
	}

	Mp4AtomPtr stsd = atom->FindAtom("mdia.minf.stbl.stsd");
	if (stsd == NULL) {
		return 1;
	}

	UINT channels = stsd->GetPropertyInt("mp4a.channels");
	if (channels > 0) {
		return channels;
	}

	return 1;
}

/** 返回 Chunk 总共的数量. */
UINT Mp4Track::GetChunkCount()
{
	return fSampleTable.GetChunkCount();
}

LPCSTR Mp4Track::GetCodecName()
{
	Mp4AtomPtr atom = GetTrackAtom();
	if (atom == NULL) {
		return "";
	}

	BYTE adpcm[] = {0x6D, 0x73, 0x00, 0x02, 0x00};
	BYTE adpcm_ima[] = {0x6D, 0x73, 0x00, 0x11, 0x00};

	Mp4AtomPtr stsd = atom->FindAtom("mdia.minf.stbl.stsd");
	if (stsd == NULL) {
		return "";
	}

	if (stsd->GetChildAtom("avc1") != NULL) {
		return "H264";

	} else if (stsd->GetChildAtom("mp4a") != NULL) {
		return "mpeg4-generic";

	} else if (stsd->GetChildAtom("samr") != NULL) {
		return "AMR";

	} else if (stsd->GetChildAtom("ulaw") != NULL) {
		return "PCMU";

	} else if (stsd->GetChildAtom("alaw") != NULL) {
		return "PCMA";

	} else if (stsd->GetChildAtom("g726") != NULL) {
		Mp4AtomPtr audioAtom = stsd->GetChildAtom("g726");
		UINT bitrate = audioAtom->GetPropertyInt("reserved2");
		if (bitrate == 40) {
			return "G726-40";

		} else if (bitrate == 32) {
			return "G726-32";

		} else if (bitrate == 24) {
			return "G726-24";

		} else if (bitrate == 16) {
			return "G726-16";

		} else {
			return "G726-16";
		}

	} else if (stsd->GetChildAtom((char*)adpcm) != NULL) {
		return "ADPCM";

	} else if (stsd->GetChildAtom((char*)adpcm_ima) != NULL) {
		return "ADPCM-IMA";
	}

	return "";
}

/** 返回指定的 track 的帧率. */
UINT Mp4Track::GetFrameRate()
{
	INT64 duration = GetMediaDuration();
	duration = Mp4Utils::Mp4TimeScale(duration, GetTimeScale(), 1000);
	if (duration <= 0) {
		return 0;
	}

	double samples = GetSampleCount();
	return UINT(samples * 1000 / duration);
}


/** 返回这个 Track 的最大码率. */
UINT Mp4Track::GetMaxBitrate()
{
	return (UINT)(GetAvgBitrate() * 1.2f);
}

/** 返回最大的 Sample 的大小. */
UINT Mp4Track::GetMaxSampleSize()
{
	return fSampleTable.GetSampleMaxSize();
}

/** 返回这个 Track 的总共的时间长度, 单位和 TimeScale 有关. */
INT64 Mp4Track::GetMediaDuration()
{
	return fTrackAtom ? fTrackAtom->GetPropertyInt("mdia.mdhd.duration") : 0; 
}

LPCSTR Mp4Track::GetMediaType()
{
	LPCSTR type = GetType();
	if (type == NULL) {
		return "";

	} else if (!strcmp(type, "vide")) {
		return "video";

	} else if (!strcmp(type, "soun")) {
		return "audio";
	}

	return "";
}

/** 
 * 返回指定的类型的参数集的内容. 
 * @param type 参数集的类型, 0 表示序列参数集, 1 表示图像参数集
 * @param index 参数集的索引, 从 0 开始.
 * @param length 返回相应的参数集的数据内容的长度, 单位为字节.
 * @return 返回相应的参数集的数据内容
 */
BYTE* Mp4Track::GetParamSets(int type, int index, WORD& length )
{
	Mp4AvcConfig avcc(GetAvcCAtom());
	if (type == 0) {
		return avcc.GetSequenceParameters(index, length);

	} else if (type == 1) {
		return avcc.GetPictureParameters(index, length);
	}

	return NULL;
}

/** 返回 Sample 的总数. */
UINT Mp4Track::GetSampleCount()
{
	return fSampleTable.GetSampleCount();
}

/**
 * 返回指定的文件偏移位置所在的 Sample 的 ID.
 * @param fileOffset 文件偏移位置, 单位为字节.
 * @return 如果在存则返回这个 Sample 的 ID, 否则返回 0.
 */
UINT Mp4Track::GetSampleIdByOffset(INT64 fileOffset)
{
	return fSampleTable.GetSampleIdByOffset(fileOffset);
}

/**
 * 返回指定的时间对应的 sample 的 ID. 
 * @param when 文件相对时间.
 * @param wantSyncSample 是否希望返回同步 Sample 的 ID.
 * @return 返回找到的 Sample 的 ID. 
 */
UINT Mp4Track::GetSampleIdByTime( INT64 when, BOOL wantSyncSample /*= false*/ )
{
	return fSampleTable.GetSampleIdByTime(when, wantSyncSample);
}

/**
 * 返回指定的 ID 的 Sample 的大小.
 * @param sampleId 这个 Sample 的 ID.
 * @return Sample 的大小. 
 */
UINT Mp4Track::GetSampleSize( UINT sampleId )
{
	return fSampleTable.GetSampleSize(sampleId);
}

/**
 * 返回指定的 ID 的 sample 在文件中的偏移位置.
 * @param sampleId Sample 的 ID.
 * @return INT64 
 */
INT64 Mp4Track::GetSampleOffset( UINT sampleId )
{
	return fSampleTable.GetSampleOffset(sampleId);
}

Mp4SampleTable& Mp4Track::GetSampleTable()
{
	return fSampleTable;
}

/**
 * 返回指定的 ID 的 Sample 的时间信息. 
 * @param sampleId 这个 Sample 的 ID.
 * @param pStartTime 输出参数, 返回这个 Sample 的开始时间.
 * @param pDuration 输出参数, 返回这个 Sample 的持续时间.
 * @return 如果成功则返回 0, 否则返回 -1. 
 */
int Mp4Track::GetSampleTimes( UINT sampleId, INT64* pStartTime, INT64* pDuration )
{
	return fSampleTable.GetSampleTimes(sampleId, pStartTime, pDuration);
}

UINT Mp4Track::GetSyncSample( UINT index )
{
	return fSampleTable.GetSyncSample(index);
}

/** 返回这个 Track 的时间比例值, 用来表示 Duraction 的时间单位. */
UINT Mp4Track::GetTimeScale()
{
	return fTimeScale ? (UINT)fTimeScale->GetValueInt() : 0;
}

/** 返回相应的 Track Atom 的指针. */
Mp4AtomPtr Mp4Track::GetTrackAtom()
{
	return fTrackAtom;
}

INT64 Mp4Track::GetTrackDuration()
{
	return fTrackAtom ? fTrackAtom->GetPropertyInt("tkhd.duration") : 0; 
}

/** 返回这个 track 的 ID. 返回 0 表示无效的 ID, 有效值从 1 起. */
UINT Mp4Track::GetTrackId()
{
	return fTrackAtom ? fTrackAtom->GetPropertyInt("tkhd.trackId") : 0; 
}

/** 返回这个 track 的类型. */
LPCSTR Mp4Track::GetType()
{
	return fTrackAtom ? (LPCSTR)fTrackAtom->GetPropertyBytes("mdia.hdlr.handlerType") : "";
}

/** 返回视频的高度. */
UINT Mp4Track::GetVideoHeight()
{
	return fTrackAtom ? (UINT)fTrackAtom->GetPropertyFloat("tkhd.height") : 0; 
}

/** 返回视频的宽度. */
UINT Mp4Track::GetVideoWidth()
{
	return fTrackAtom ? (UINT)fTrackAtom->GetPropertyFloat("tkhd.width") : 0;
}

/** 初始化这个 Track. */
void Mp4Track::Init()
{
	Mp4AtomPtr track = fTrackAtom;
	if (track == NULL) {
		return;
	}

	fTimeScale	= track->FindProperty("mdia.mdhd.timeScale");
	fSampleTable.Init(track);
}

BOOL Mp4Track::IsSyncSample( UINT sampleId )
{
	return fSampleTable.IsSyncSample(sampleId);
}

/** 重新初始化所有的成员变量. */
void Mp4Track::Reset()
{
	fTimeScale	= NULL;
	fSampleTable.Reset();
}

/** 设置这个 Track 的总共的时间长度, 单位和 TimeScale 有关. */
void Mp4Track::SetMediaDuration( INT64 duration )
{
	if (fTrackAtom != NULL) {
		fTrackAtom->SetPropertyInt("mdia.mdhd.duration", duration);
	}
}

void Mp4Track::SetTrackDuration( INT64 duration, UINT timeScale )
{
	if (fTrackAtom != NULL) {
		UINT trackTimeScale = GetTimeScale();
		if (trackTimeScale > 0 && trackTimeScale != timeScale) {
			duration = duration * timeScale / trackTimeScale;
		}
		fTrackAtom->SetPropertyInt("tkhd.duration", duration);
	}
}

/** 设置这个 Track 的时间比例值, 用来表示 Duraction 的时间单位. */
void Mp4Track::SetTimeScale( UINT timeScale )
{
	if (fTimeScale) {
		fTimeScale->SetValueInt(timeScale);
	}
}

/** 绑定到指定的 Trak ATOM. */
void Mp4Track::SetTrackAtom( Mp4Atom* track )
{
	fTrackAtom = track;
	Init();
}

/** 设置这个 track 的 ID 值, 有效值从 1 起. */
void Mp4Track::SetTrackId( UINT id )
{
	if (fTrackAtom) {
		fTrackAtom->SetPropertyInt("tkhd.trackId", id);
	}
}

/** 设置这个 track 的类型. */
void Mp4Track::SetType( LPCSTR type )
{
	if (fTrackAtom && type) {
		fTrackAtom->SetPropertyString("mdia.hdlr.handlerType", type);
	}
}

int Mp4Track::UpdateChunkOffset( UINT offset )
{
	return fSampleTable.UpdateChunkOffset(offset);
}

}
