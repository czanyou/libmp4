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
#ifndef _NS_VISION_MP4_WRITER_H
#define _NS_VISION_MP4_WRITER_H

#include "mp4common.h"
#include "mp4atom.h"
#include "mp4track.h"
#include "mp4metawriter.h"
#include "mp4stream.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4SampleFlags

/** Sample 标志. */
enum Mp4SampleFlags 
{
	SAMPLE_SYNC_POINT	= 0x01,		///< 这是一个同步点
	SAMPLE_START		= 0x10,		///< 这是这个 sample 的第一个数据包
	SAMPLE_END			= 0x20,		///< 这是这个 sample 的最后一个数据包
	SAMPLE_FRAGMENT		= 0x80,		///< 这个数据包是一个碎片
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Writer

/** 
 * 代表一个 MP4 文件复用器/生成器. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Writer : public MediaMuxer 
{
public:
	Mp4Writer();
	virtual ~Mp4Writer();

// Attributes -------------------------------------------------
public:
	INT64 GetCreationTime();
	INT64 GetDuration();
	INT64 GetLength();
	UINT  GetTimeScale();
	BOOL  IsOpen();

	void  SetCreationTime(INT64 creationTime);
	int   SetOutputFilename(LPCSTR filename);

// Operations -------------------------------------------------
public:
	int  AddGpsTrackData(LPCSTR name, LPCSTR track);
	int  BeginWriting();
	int  Close();
	int  EndStreaming();
	int  EndWriting();
	int  Flush();
	int  Open(UINT flags = 0);
	int  Reset();

	int SetAudioFormat(UINT timeScale, INT64 duration, int channels, LPCSTR type, int param = 0);
	int SetGpsTrack(LPCSTR track);
	int SetVideoParamSets(char* pps, char* sqs);
	int SetVideoFormat(UINT timeScale, INT64 duration, WORD width, WORD height, LPCSTR type);

	int  WriteAudioSample (MediaFileSample* sample);
	int  WriteVideoFragment(MediaFileSample* sample, BYTE nalHeader);
	int  WriteVideoSample (MediaFileSample* sample);

// Implementation ---------------------------------------------
protected:
	Mp4TrackPtr AddAudioTrack();
	Mp4TrackPtr AddVideoTrack();
	Mp4AtomPtr  AddTrack(LPCSTR type, UINT timeScale = 1000);

protected:
	int  AddChunk(Mp4TrackInfo* track);
	int  AddSample(Mp4TrackInfo* track, MediaFileSample* sample);
	int  AddVideoParamSets(Mp4Track* track);
	int  CheckFile();
	int  FilterParamSets(BYTE* sample, UINT sampleSize);
	int  FinishMoovAtom();
	int  FinishTrack( Mp4TrackInfo* writeTrack, Mp4Track* track );

// Data Members -----------------------------------------------
private:	
	Mp4TrackInfoPtr fAudioTrackInfo;	///< 音频 Track 属性
	Mp4TrackInfoPtr fVideoTrackInfo;	///< 视频 Track 属性

	Mp4MetaWriter fMetaWriter;		///< 
	Mp4Stream fMp4Stream;			///<

private:	
	INT64   fCreationTime;			///< 这个文件的创建时间, 取自第一帧的采集时间
	INT64   fDuration;				///< 
	char	fFileName[MAX_PATH];	///< 这个媒体文件的文件名
	BOOL	fIsAACAudio;			///< 指出是否是 AAC 音频
	BOOL    fMetaWrited;			///< 指出索引元数据信息已写入
	INT64   fModificationTime;		///< 
	BOOL	fMoiveFinished;			///< 指出这个媒体文件已经完成了

	UINT    fTrackCount;			///< Track 数量
	BOOL	fVideoIsSyncPoint;		///< 指出当前视频帧是否是同步点
	UINT	fVideoSyncPointCount;	///< 已经写入的视频关键帧的数目
	UINT	fVideoSampleSize;		///< 视频 Sample 缓存区有效数据大小
	BOOL	fWaitSyncPoint;			///< 是否需要等待视频同步点
};	

typedef SmartPtr<Mp4Writer> Mp4WriterPtr;

};

#endif // !defined(_NS_VISION_MP4_WRITER_H)
