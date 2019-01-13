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
#ifndef _NS_VISION_MP4_META_WRITER_H
#define _NS_VISION_MP4_META_WRITER_H

#include "mp4common.h"
#include "mp4atom.h"
#include "mp4track.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4TrackInfo

/**
 * 代表一个 Track
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4TrackInfo : public VisionBaseObject
{
public:
	Mp4TrackInfo();
	VISION_BASEREF_METHODS(Mp4TrackInfo);	///< 实现引用计数功能.

// Attributes -------------------------------------------------
public:
	Mp4Chunk& GetChunk() { return fChunk; }
	UINT GetDuration(INT64 timestamp, UINT timeScale );
	void Clear();

// Data Members -----------------------------------------------
public:
	Mp4Chunk fChunk;			///< 当前 Chunk
	UINT     fChunkCount;		///< 

	UINT	 fChannels;			///< 当前 Track 的音频通道数
	UINT	 fCodecType;		///< 当前 Track 的编码类型
	INT64	 fDuration;			///< 当前 Track 的时间总长度
	INT64	 fLastTimestamp;	///< 当前 Track 前一帧的时间戳
	UINT	 fParams;			///< 当前 Track 的编码参数

	UINT     fSampleCount;		///< 当前 Track 的 Sample 数
	UINT     fTimeScale;		///< 当前 Track 的 Time Scale
	UINT     fTrackId;			///< 当前 Track 的 ID
	UINT	 fVideoHeight;		///< 当前 Track 的视频高度
	UINT	 fVideoWidth;		///< 当前 Track 的视频宽度

	Mp4SizeTableProperty fSqsSets;
	Mp4SizeTableProperty fPpsSets;
};

typedef SmartPtr<Mp4TrackInfo> Mp4TrackInfoPtr;
typedef Mp4Array<Mp4TrackInfoPtr> Mp4TrackInfoArray;

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4MetaWriter

class Mp4Writer;

/** 
 * 代表一个 MP4 文件复用器/生成器. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4MetaWriter
{
public:
	Mp4MetaWriter();
	~Mp4MetaWriter();

// Operations -------------------------------------------------
public:
	int  Close();

	void LoadIndexInfo( Mp4Track* videoTrack, Mp4Track* audioTrack );
	void LoadMetaInfo( Mp4Writer* writer);
	int  LoadTrackMeta( char* str, Mp4Writer* writer );
	int  LoadFileMeta( char* str, Mp4Writer* writer );

	int  Open(LPCSTR name, UINT flags);
	void SetFileName(LPCSTR name);

	void WriteIndexInfo(LPCSTR fmt, ...);
	int  WriteMetaInfo(INT64 createTime);
	void WriteAudioMeta( Mp4TrackInfo* fAudioTrack );
	void WriteVideoMeta( Mp4TrackInfo* fVideoTrack );

// Data Members -----------------------------------------------
private:	
	Mp4FilePtr	fIndexFile;			///< Mp4 索引文件
	char	fFileName[MAX_PATH];	///< 这个媒体文件的文件名
	UINT	fVideoTrackId;
	UINT	fAudioTrackId;

};

};

#endif // !defined(_NS_VISION_MP4_META_WRITER_H)
