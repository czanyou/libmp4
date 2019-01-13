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
#include "mp4writer.h"
#include "mp4avc.h"
#include "mp4avcc.h"
#include "mp4metawriter.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4TrackInfo class

Mp4TrackInfo::Mp4TrackInfo()
{
	fChannels		= 0;
	fCodecType		= 0;
	fDuration		= 0;
	fLastTimestamp	= -1;
	fParams			= 0;

	fSampleCount	= 0;
	fTimeScale		= 0;
	fTrackId		= 0;
	fVideoHeight	= 0;
	fVideoWidth		= 0;

	fChunkCount		= 0;
}

/** */
void Mp4TrackInfo::Clear()
{
	fChannels		= 0;
	fCodecType		= 0;
	fDuration		= 0;
	fLastTimestamp	= -1;
	fParams			= 0;

	fSampleCount	= 0;
	fTimeScale		= 0;
	fTrackId		= 0;
	fVideoHeight	= 0;
	fVideoWidth		= 0;

	fChunkCount		= 0;
	fChunk.Clear();
}

/** */
UINT Mp4TrackInfo::GetDuration(INT64 timestamp, UINT timeScale )
{
	INT64 duration = 0;
	if (fLastTimestamp >= 0) {
		duration = timestamp - fLastTimestamp;
	}

	fLastTimestamp = timestamp;

	// 换算时间戳
	if (timeScale == 0) {
		timeScale = 1000;
	}

	if (timeScale != fTimeScale) {
		duration = (duration * fTimeScale) / timeScale;
	}	

	return (UINT)duration;
}

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4MetaWriter class

Mp4MetaWriter::Mp4MetaWriter()
{
	memset(fFileName, 0, sizeof(fFileName));

	fAudioTrackId	= 2;
	fVideoTrackId	= 1;
}

Mp4MetaWriter::~Mp4MetaWriter()
{
	Close();
}

int Mp4MetaWriter::Close()
{
	if (fIndexFile) {
		fIndexFile->Close();
		fIndexFile = NULL;
	}

	return 0;
}

int Mp4MetaWriter::Open( LPCSTR name, UINT flags )
{
	int ret = 0;

	strncpy(fFileName, name, MAX_PATH);

	// Movie index file
	fIndexFile  = new Mp4File();
	if ((ret = fIndexFile->Open(fFileName, "wb")) != MP4_S_OK) {
		fIndexFile = NULL;
		Close();
		return ret;
	}

	return 0;
}

int Mp4MetaWriter::LoadFileMeta( char* str, Mp4Writer* writer )
{
	if (str == NULL || writer == NULL) {
		return -1;
	}

	fAudioTrackId	= 2;
	fVideoTrackId	= 1;

	while (true) {
		char* end = strchr(str, ';');
		if (end) {
			*end = '\0';
		}

		// start time
		if (strncmp(str, "start=", 6) == 0) { // Start time
			INT64 creationTime = atoll(str + 6);
			writer->SetCreationTime(creationTime + 2082844800);

			//LOG_D("%u\r\n", (UINT)creationTime);
		}

		if (end == NULL) {
			break;
		}
		str = end + 1;
	}

	return 0;
}

void Mp4MetaWriter::LoadIndexInfo( Mp4Track* videoTrack, Mp4Track* audioTrack )
{
	FILE* indexFile = fopen(fFileName, "r");
	if (indexFile == NULL) {
		return;
	}

	//LOG_D("%s\r\n", fFileName);

	char line[MAX_PATH + 1];
	memset(line, 0, sizeof(line));

	Mp4TrackPtr track = NULL;

	while (true) {
		char* p = fgets(line, MAX_PATH, indexFile);
		if (p == NULL) {
			break;
		}

		if (line[0] == '$') {	// Meta info
			continue;

		} else if (line[0] == '@') { // Chunk info
			UINT chunkId = 0, sampleId = 0, offset = 0, trackId = 0, count = 0;
			sscanf(line, "@%u,%u,%u,%u,%u", &trackId, &chunkId, &sampleId, &offset, &count);
			if (videoTrack && trackId == fVideoTrackId) {
				track = videoTrack;
				track->AddChunk(sampleId, count, offset);

			} else if (audioTrack && trackId == fAudioTrackId) {
				track = audioTrack;
				track->AddChunk(sampleId, count, offset);
			}

		} else if (line[0] == '!') {	// Sync Point (sample)
			UINT size = 0, duration = 0;
			sscanf(line, "!%u,%u", &size, &duration);
			if (track) {
				track->AddSample(TRUE, size, duration);
			}

		} else { // Sample
			UINT size = 0, duration = 0;
			int ret = sscanf(line, "%u,%u", &size, &duration);
			if (ret >= 2 && track) {
				track->AddSample(FALSE, size, duration);
			}
		}
	}

	fclose(indexFile);
	indexFile = NULL;
}

void Mp4MetaWriter::LoadMetaInfo(Mp4Writer* writer)
{
	FILE* indexFile = fopen(fFileName, "r");
	if (indexFile == NULL) {
		return;
	}

	//LOG_D("%s\r\n", fFileName);

	char line[MAX_PATH + 1];
	memset(line, 0, sizeof(line));

	while (true) {
		char* p = fgets(line, MAX_PATH, indexFile);
		if (p == NULL) {
			break;
		}

		if (line[0] != '$') {	// Meta info
			break;
		}

		if (strncmp(line, "$file:", 6) == 0) { // file meta info
			LoadFileMeta(line + 6, writer);

		} else if (strncmp(line, "$track:", 7) == 0) { // track meta info
			LoadTrackMeta(line + 7, writer);
		}
	}

	fclose(indexFile);
	indexFile = NULL;
}

int Mp4MetaWriter::LoadTrackMeta( char* str, Mp4Writer* writer )
{
	if (str == NULL || writer == NULL) {
		return -1;
	}

	int trackId = atoi(str);

	int type = -1;
	int width = 0, height = 0, channels = 0, param = 0, timeScale = 0;
	char* sqsText = NULL;
	char* ppsText = NULL;

	while (true) {
		char* end = strchr(str, ';');
		if (end) {
			*end = '\0';
		}

		if (strncmp(str, "type=video", 10) == 0) {
			type = MEDIA_TRACK_VIDEO;

		} else if (strncmp(str, "type=audio", 10) == 0) {
			type = MEDIA_TRACK_AUDIO;

		} else if (strncmp(str, "timeScale=", 10) == 0) {
			timeScale = atoi(str + 10);

		} else if (strncmp(str, "width=", 6) == 0) {
			width = atoi(str + 6);

		} else if (strncmp(str, "height=", 7) == 0) {
			height = atoi(str + 7);

		} else if (strncmp(str, "channels=", 9) == 0) {
			channels = atoi(str + 9);

		} else if (strncmp(str, "param=", 6) == 0) {
			param = atoi(str + 6);

		} else if (strncmp(str, "sqs=", 4) == 0) {
			sqsText = str + 4;

		} else if (strncmp(str, "pps=", 4) == 0) {
			ppsText = str + 4;
		}

		if (end == NULL) {
			break;
		}

		str = end + 1;
	}

	//LOG_D("%d, (%dx%d)\r\n", type, width, height);

	// Set track info
	if (type == MEDIA_TRACK_VIDEO) {
		writer->SetVideoFormat(timeScale, 0, width, height, 0);
		writer->SetVideoParamSets(ppsText, sqsText);
		fVideoTrackId = trackId;

	} else if (type == MEDIA_TRACK_AUDIO) {
		writer->SetAudioFormat(timeScale, 0, channels, NULL, param);
		fAudioTrackId = trackId;
	}	

	return 0;
}

void Mp4MetaWriter::SetFileName( LPCSTR name )
{
	if (name) {
		strncpy(fFileName, name, MAX_PATH);
	}
}

void Mp4MetaWriter::WriteAudioMeta( Mp4TrackInfo* audioTrack )
{
	if (audioTrack == NULL) {
		return;
	}

	WriteIndexInfo("$track:%u;type=audio;",	audioTrack->fTrackId);
	WriteIndexInfo("codec=%u;",				audioTrack->fCodecType);
	WriteIndexInfo("timeScale=%u;",			audioTrack->fTimeScale);
	WriteIndexInfo("channels=%u;",			audioTrack->fChannels);
	WriteIndexInfo("param=%u;\n",			audioTrack->fParams);
}

void Mp4MetaWriter::WriteIndexInfo( LPCSTR fmt, ... )
{
	va_list ap;
	char buffer[MAX_PATH + 1];

	va_start(ap, fmt);
	int size = vsnprintf(buffer, MAX_PATH, fmt, ap);
	va_end(ap);

	if (size >= 0) {
		buffer[size] = '\0';
	}

	if (fIndexFile) {
		fIndexFile->WriteBytes((BYTE*)buffer, size);
	}
}

int Mp4MetaWriter::WriteMetaInfo(INT64 createTime)
{
	// File meta info
	WriteIndexInfo("$file:*;start=%u\n", (UINT)createTime);
	return 0;
}

void Mp4MetaWriter::WriteVideoMeta( Mp4TrackInfo* videoTrack )
{
	if (videoTrack == NULL) {
		return;
	}

	WriteIndexInfo("$track:%u;type=video;",	videoTrack->fTrackId);
	WriteIndexInfo("codec=H264;");
	WriteIndexInfo("timeScale=%u;",			videoTrack->fTimeScale);
	WriteIndexInfo("width=%u;",				videoTrack->fVideoWidth);
	WriteIndexInfo("height=%u;",			videoTrack->fVideoHeight);

	// SQS 参数集
	WORD sqsLength = 0;
	BYTE* sqsData = videoTrack->fSqsSets.GetEntry(0, sqsLength);
	if (sqsData) {
		char sqsText[MAX_PATH];
		memset(sqsText, 0, sizeof(sqsText));
		Mp4Utils::HexEncode(sqsData, sqsLength, sqsText, MAX_PATH);
		WriteIndexInfo("sqs=%s;", sqsText);
	}

	// PPS 参数集
	WORD ppsLength = 0;
	BYTE* ppsData = videoTrack->fPpsSets.GetEntry(0, ppsLength);
	if (ppsData) {
		char ppsText[MAX_PATH];
		memset(ppsText, 0, sizeof(ppsText));
		Mp4Utils::HexEncode(ppsData, ppsLength, ppsText, MAX_PATH);
		WriteIndexInfo("pps=%s;", ppsText);
	}

	WriteIndexInfo("\n");
}

};
