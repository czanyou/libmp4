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
#include "mp4writer.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////

/** 构建一个新的 Mp4Sample. */
MediaFileSample::MediaFileSample()
{
	fData		= NULL;
	fSize		= 0;
	fMaxSize	= 0;

	fTrackIndex	= 0;
	fSampleId	= 0;

	fDuraction	= 0;
	fIsSyncPoint		= FALSE;
	fIsEnd		= TRUE;
	fTimestamp	= 0;
	fTimeScale	= 0;
}

MediaFileSample::MediaFileSample( const MediaFileSample& ths )
{
	if (this == &ths) {
		return;
	}

	fData		= ths.fData;
	fSize		= ths.fSize;
	fMaxSize	= ths.fMaxSize;

	fTrackIndex	= ths.fTrackIndex;
	fSampleId	= ths.fSampleId;

	fDuraction	= ths.fDuraction;
	fIsSyncPoint		= ths.fIsSyncPoint;
	fIsEnd		= ths.fIsEnd;
	fTimestamp	= ths.fTimestamp;
	fTimeScale	= ths.fTimeScale;
}

MediaFileSample::~MediaFileSample()
{
	
}

/** 清除这个 Mp4Sample 的内容, 重置所有的属性的值. */
void MediaFileSample::Clear()
{
	fTrackIndex	= 0;
	fSampleId	= 0;

	fDuraction	= 0;
	fIsSyncPoint		= FALSE;
	fIsEnd		= TRUE;
	fTimestamp	= 0;
	fTimeScale	= 0;
}

MediaFileSample& MediaFileSample::operator=( const MediaFileSample& ths )
{
	if (this == &ths) {
		return *this;
	}

	fData		= ths.fData;
	fSize		= ths.fSize;
	fMaxSize	= ths.fMaxSize;

	fTrackIndex	= ths.fTrackIndex;
	fSampleId	= ths.fSampleId;

	fDuraction	= ths.fDuraction;
	fIsSyncPoint		= ths.fIsSyncPoint;
	fIsEnd		= ths.fIsEnd;
	fTimestamp	= ths.fTimestamp;
	fTimeScale	= ths.fTimeScale;

	return *this;
}

/** 创建一个媒体文件阅读器. */
MediaExtractorPtr MediaCreateExtractor()
{
	return new Mp4Reader();
}

/** 创建一个媒体文件书写器. */
MediaMuxerPtr MediaCreateMuxer()
{
	return new Mp4Writer();
}

}
