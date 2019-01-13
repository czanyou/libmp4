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

namespace mp4 {

const UINT kMp4MaxFrameSize	= 1024 * 1024 * 2;

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Writer class


/** 构建一个新的 Mp4Writer. */
Mp4Writer::Mp4Writer()
{
	fCreationTime			= 0;
	fModificationTime		= 0;
	fIsAACAudio				= FALSE;
	fVideoIsSyncPoint		= TRUE;

	fVideoSyncPointCount	= 0;
	fVideoSampleSize		= 0;
	fWaitSyncPoint			= TRUE;
	fTrackCount				= 0;
	fMetaWrited				= FALSE;
	fMoiveFinished			= FALSE;
	fDuration				= 0;

	memset(fFileName, 0, sizeof(fFileName));
}

Mp4Writer::~Mp4Writer()
{
	fMp4Stream.Close();
	fMetaWriter.Close();
}

/** 添加一个音频 Track. */
Mp4TrackPtr Mp4Writer::AddAudioTrack()
{
	Mp4TrackInfoPtr trackInfo = fAudioTrackInfo;
	if (trackInfo == NULL) {
		return NULL;
	}

	UINT timeScale	= trackInfo->fTimeScale;
	UINT channels	= trackInfo->fChannels;
	UINT param		= trackInfo->fParams;
	UINT atomId		= trackInfo->fCodecType;
	UINT trackId	= trackInfo->fTrackId;

	char type[5];
	Mp4Utils::Mp4AtomType(atomId, type);

	Mp4AtomPtr trakAtom = AddTrack("soun", timeScale);
	if (trakAtom == NULL) {
		return NULL;
	}

	Mp4TrackPtr track = new Mp4Track();
	track->SetTrackAtom(trakAtom);
	track->SetTimeScale(timeScale);

	trakAtom->AddChildAtom("mdia.minf.smhd", 0);
	trakAtom->SetPropertyFloat("tkhd.volume", 1.0);
	trakAtom->SetPropertyInt("tkhd.trackId",  trackId);	

	Mp4AtomPtr stsd = trakAtom->FindAtom("mdia.minf.stbl.stsd");
	if (stsd == NULL) {
		return NULL;
	}

	stsd->SetPropertyInt("entryCount", 1);
	Mp4AtomPtr atom = stsd->AddChildAtom(type);
	if (atom == NULL) {
		return NULL;
	}

	// Audio props
	if (atomId == Mp4Utils::AtomId("mp4a")) {
		atom->SetPropertyInt("sampleRate", timeScale);
		atom->SetPropertyInt("channels", channels);
		fIsAACAudio = TRUE;

	} else if (atomId == Mp4Utils::AtomId("sawb")) {
		atom->SetPropertyInt("timeScale", timeScale);
		atom->SetPropertyInt("damr.modeSet", 0);
		atom->SetPropertyInt("damr.modeChangePeriod", 0);
		atom->SetPropertyInt("damr.framesPerSample", param);

	} else if (atomId == Mp4Utils::AtomId("samr")) {
		atom->SetPropertyInt("timeScale", timeScale);
		atom->SetPropertyInt("damr.modeSet", 0);
		atom->SetPropertyInt("damr.modeChangePeriod", 0);
		atom->SetPropertyInt("damr.framesPerSample", param);

	} else {
		atom->SetPropertyInt("dataReferenceIndex", 1);
		atom->SetPropertyInt("sampleRate", timeScale);
		atom->SetPropertyInt("channels", channels);
		atom->SetPropertyInt("sampleSize", 16);
		atom->SetPropertyInt("reserved2", param);
	}

	track->Init();
	return track;
}

/**
 * 把 Chunk 缓存区中的数据写入文件中.
 * @param file 要写入的文件
 * @return 如果成功则返回 0. 
 */
int Mp4Writer::AddChunk(Mp4TrackInfo* track)
{
	Mp4FilePtr file = fMp4Stream.GetFile();
	if (file == NULL || track == NULL) {
		return MP4_ERR_FAILED;
	} 

	if (!fMetaWrited) {
		fMetaWriter.WriteMetaInfo(GetCreationTime());
		fMetaWriter.WriteVideoMeta(fVideoTrackInfo);
		fMetaWriter.WriteAudioMeta(fAudioTrackInfo);
		fMetaWrited = TRUE;
	}

	// Write chunk buffer
	Mp4Chunk& chunk = track->GetChunk();
	BYTE* buffer = chunk.GetBuffer();
	UINT  length = chunk.GetLength();
	if (buffer == NULL || length <= 0) {
		chunk.Reset();
		return MP4_ERR_FAILED;
	}

	INT64 chunkOffset = file->GetPosition();
	UINT result = file->WriteBytes(buffer, length);
	if (result != length) {
		return MP4_ERR_WRITE;
	}

	// Add a chunk
	UINT sampleCount = chunk.GetSampleCount();
	UINT firstSample = chunk.GetFirstSample();
	UINT trackId = track->fTrackId;
	UINT chunkId = track->fChunkCount + 1;
	fMetaWriter.WriteIndexInfo("@%u,%u,%u,%u,%u\n", 
		trackId, chunkId, firstSample, (UINT)chunkOffset, sampleCount);

	// Add samples
	MediaFileSampleArray& samples = chunk.GetSamples();
	for (UINT i = 0, count = samples.GetCount(); i < count; i++) {
		MediaFileSamplePtr sample = samples[i];
		if (sample == NULL) {
			break;
		}

		UINT duration	= (UINT)sample->GetDuration();
		BOOL isSync		= sample->IsSyncPoint();
		UINT sampleSize = sample->GetSize();

		if (isSync) {
			fMetaWriter.WriteIndexInfo("!%u,%u\n", sampleSize, duration);

		} else {
			fMetaWriter.WriteIndexInfo("%u,%u\n", sampleSize, duration);
		}
	}

	chunk.Reset();

	CheckFile();
	return MP4_S_OK;
}

int Mp4Writer::AddGpsTrackData( LPCSTR name, LPCSTR track )
{
	if (Mp4IsEmpty(name) || Mp4IsEmpty(track)) {
		return 0;
	}

	Close();

	int ret = 0;

	strncpy(fFileName, name, MAX_PATH);
	Mp4FilePtr file = new Mp4File();
	if ((ret = file->Open(fFileName, "r+")) != MP4_S_OK) {
		file = NULL;	
		Close();
		return ret; // 文件打开失败
	}

	// Movie stream
	fMp4Stream.OpenWrite(file, 2);
	fMp4Stream.LoadGpsAtom();

	Mp4AtomPtr moov = fMp4Stream.GetMoovAtom();
	Mp4AtomPtr mdat = fMp4Stream.GetMdatAtom();
	if (moov == NULL || mdat == NULL) {
		Close();
		return -1;
	}

	Mp4AtomPtr atom = moov->GetChildAtom("gps ");
	if (atom) {
		Close();
		return 0;
	}

	atom = fMp4Stream.GetGpsTrackAtom(TRUE);
	if (atom) {
		atom->SetPropertyString("track", track);
	}

	INT64 pos = moov->GetEnd();
	if (pos < mdat->GetEnd()) {
		pos = mdat->GetEnd();
	}
	file->SetPosition(pos);
	atom->Write(file);

	moov->WriteSize(file);

	Close();
	return 0;
}

/** 添加一个 Sample. */
int Mp4Writer::AddSample(Mp4TrackInfo* track, MediaFileSample* sample)
{
	if (track == NULL || sample == NULL) {
		return MP4_ERR_FAILED;
	}

	sample->SetSampleId(track->fSampleCount + 1);
	sample->SetTrackIndex(track->fTrackId);
	track->fDuration += sample->GetDuration();

	Mp4Chunk& chunk = track->GetChunk();
	chunk.AddSample(sample);
	if (chunk.IsFull()) {
		AddChunk(track);	// 添加一个 Chunk
		track->fChunkCount++;
	}

	track->fSampleCount++;
	return 0;
}

/** 
 * 添加一个指定的类型的 Track. 
 * @param type 要添加的 Track 的类型, 如 "vide", "soun".
 * @param timeScale Time Scale, 默认为 1000.
 * @return 返回添加的 track 对象的指针.
 */
Mp4AtomPtr Mp4Writer::AddTrack( LPCSTR type, UINT timeScale /*= 1000*/ )
{
	if (type == NULL || *type == '\0') {
		return NULL;
	}

	// 创建 trak 节点
	Mp4AtomPtr trakAtom = fMp4Stream.AddTrackAtom();
	if (trakAtom == NULL) {
		return NULL;
	}

	trakAtom->SetPropertyInt("tkhd.creationTime",			fCreationTime);
	trakAtom->SetPropertyInt("tkhd.modificationTime",		fModificationTime);
	trakAtom->SetPropertyInt("mdia.mdhd.creationTime",		fCreationTime);
	trakAtom->SetPropertyInt("mdia.mdhd.modificationTime",	fModificationTime);
	trakAtom->SetPropertyString("mdia.hdlr.handlerType",	type);		
	
	return trakAtom;
}

/** 添加参数集信息. */
int Mp4Writer::AddVideoParamSets(Mp4Track* track)
{
	if (track == NULL || fVideoTrackInfo == NULL) {
		return 0;
	}

	// SQS
	WORD sqsLength = 0;
	BYTE* sqsData = fVideoTrackInfo->fSqsSets.GetEntry(0, sqsLength);
	if (sqsData) {
		Mp4Video parser;
		if (parser.ParseHeader(sqsData, sqsLength) >= 0) {
			Mp4AtomPtr trackAtom = track->GetTrackAtom();
			Mp4AtomPtr avc1 = trackAtom ? trackAtom->FindAtom("mdia.minf.stbl.stsd.avc1") : NULL;				
			if (avc1) {
				avc1->SetPropertyInt("width",  parser.GetWidth());
				avc1->SetPropertyInt("height", parser.GetHeight());
			}
		}

		Mp4AvcConfig avc(track->GetAvcCAtom());
		avc.AddSequenceParameters(sqsData, sqsLength);

		if (sqsLength > 3) {
			avc.SetProfileLevel(sqsData[1], sqsData[3]);
			avc.SetProfileCompatibility(sqsData[2]);
		}
	}

	// PPS
	WORD ppsLength = 0;
	BYTE* ppsData = fVideoTrackInfo->fPpsSets.GetEntry(0, ppsLength);
	if (ppsData) {
		Mp4AvcConfig avc(track->GetAvcCAtom());
		avc.AddPictureParameters(ppsData, ppsLength);
	}

	return 0;
}

/** 添加一个视频 Track. */
Mp4TrackPtr Mp4Writer::AddVideoTrack()
{
	Mp4TrackInfoPtr trackInfo = fVideoTrackInfo;
	if (trackInfo == NULL) {
		return NULL;
	}

	UINT timeScale	= trackInfo->fTimeScale;
	UINT width		= trackInfo->fVideoWidth;
	UINT height		= trackInfo->fVideoHeight;
	UINT trackId	= trackInfo->fTrackId;

	Mp4AtomPtr trakAtom = AddTrack("vide", timeScale);
	if (trakAtom == NULL) {
		return NULL;
	}

	Mp4TrackPtr track = new Mp4Track();
	track->SetTrackAtom(trakAtom);
	track->SetTimeScale(timeScale);

	trakAtom->AddChildAtom("mdia.minf.vmhd", 0);
	trakAtom->AddChildAtom("mdia.minf.stbl.stss"); // 同步点表格
	trakAtom->SetPropertyFloat("tkhd.width",  (float)width);
	trakAtom->SetPropertyFloat("tkhd.height", (float)height);
	trakAtom->SetPropertyInt("tkhd.trackId",  trackId);	

	Mp4AtomPtr stsd = trakAtom->FindAtom("mdia.minf.stbl.stsd");
	if (stsd == NULL) {
		return NULL;
	}

	stsd->SetPropertyInt("entryCount", 1);
	Mp4AtomPtr atom = stsd->AddChildAtom("avc1");
	if (atom) {
		atom->SetPropertyInt("width",  width);
		atom->SetPropertyInt("height", height);
	}

	track->Init();
	return track;
}

/** 
 * 准备开始写 MP4 文件. 这个方法会先将 mdat 之前的 ATOM 节点和 mdat 节点的
 * 头部先写入文件, 这样接着就可以开始写 sample 数据了, 而 mdat 之后的节点
 * 要等 mdat 写完之后, 才会在调用 FinishWrite 时再写入文件中.
 * @return 如果成功则返回 MP4_S_OK(0), 否则返回一个表示错误码的负数.
 *		- MP4_ERR_NOT_OPEN 如果文件还没有打开.
 *		- MP4_ERR_NULL_ATOM 碰到空的 ATOM 节点.
 */
int Mp4Writer::BeginWriting()
{
	return fMp4Stream.BeginWriting();
}

/** 关闭这个文件, 并释放所有的资源. */
int Mp4Writer::Close()
{
	if (fVideoTrackInfo) {
		fVideoTrackInfo->Clear();
		fVideoTrackInfo = NULL;
	}

	if (fAudioTrackInfo) {
		fAudioTrackInfo->Clear();
		fAudioTrackInfo = NULL;
	}

	fMp4Stream.Close();

	if (fMoiveFinished) {
		fMoiveFinished = FALSE;

		char filename[MAX_PATH + 1];
		memset(filename, 0, sizeof(filename));

		// movie data file
		Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mdf");
		if (access(filename, R_OK) == 0) {
			rename(filename, fFileName);

			// movie index file
			Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mif");
			remove(filename);
		} 
	}

	fMetaWriter.Close();
	Reset();

	return MP4_S_OK;
}

/** 检查文件状态. */
int Mp4Writer::CheckFile()
{
	char filename[MAX_PATH + 1];
	memset(filename, 0, sizeof(filename));

	// movie data file
	Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mdf");
	if (access(filename, 0) != 0) {
		Close();
		return -1;
	}

	// movie index file
	Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mif");
	if (access(filename, 0) != 0) {
		Close();
		return -1;
	}

	return 0;
}

int Mp4Writer::EndStreaming()
{
	Mp4TrackPtr videoTrack = AddVideoTrack();
	Mp4TrackPtr audioTrack = AddAudioTrack();

	// 保存所有缓存的还没有写入的数据
	if (fVideoTrackInfo) {
		AddChunk(fVideoTrackInfo);
	}

	if (fAudioTrackInfo) {
		AddChunk(fAudioTrackInfo);
	}

	fMp4Stream.EndMdatWriting();
	fMetaWriter.Close();

	fMetaWriter.LoadIndexInfo(videoTrack, audioTrack);

	// 写入所有的 track 还没有写入文件的缓存区的数据
	if (videoTrack) {
		FinishTrack(fVideoTrackInfo, videoTrack);
		AddVideoParamSets(videoTrack);
	}

	if (audioTrack) {
		FinishTrack(fAudioTrackInfo, audioTrack);
	}

	fDuration = videoTrack ? videoTrack->GetTrackDuration() : 0;
	fMp4Stream.SetDuration(fDuration);
	fMp4Stream.SetNextTrackId(fTrackCount + 1);

	//LOG_D("track: %u (%u)\r\n", (UINT)videoTrack->GetTrackDuration(), (UINT)fDuration);
	return 0;
}

/** 完成写操作和收尾工作. */
int Mp4Writer::EndWriting()
{
	if (fDuration <= 0) {
		EndStreaming();
	}

	FinishMoovAtom();

	fMoiveFinished = TRUE;
	return MP4_S_OK;
}

/**
 * 过滤掉序列和图像参数集等 NALU 包.
 * @param track 视频 Track 的指针.
 * @param sample 这个 Sample 的内容
 * @param sampleSize 这个 Sample 的大小.
 * @return 大于零表示已经处理. 
 */
int Mp4Writer::FilterParamSets(BYTE* sample, UINT sampleSize)
{
	if (fVideoTrackInfo == NULL || sample == NULL || sampleSize <= 0) {
		return 0;

	} else if (fVideoSyncPointCount != 0) {
		return 0; // 只解析第一个关键帧
	}

	// 取得序列参数集
	if (sample[0] == 0x00 && sample[1] == 0x00) {
		BYTE headerSize = (sample[2] == 0x01) ? 3 : 4;
		sample += headerSize;
		sampleSize -= headerSize;
	}

	BYTE naluType = Mp4Video::GetNaluType(sample);
	if (naluType == H264_NAL_TYPE_SEQ_PARAM) {
		fVideoTrackInfo->fSqsSets.AddEntry(sample, sampleSize);
		return sampleSize;

	} else if (naluType == H264_NAL_TYPE_PIC_PARAM) {
		fVideoTrackInfo->fPpsSets.AddEntry(sample, sampleSize);
		return sampleSize;
	}

	return 0;
}

/** 完成 mdat atom 及之后的节点的写操作. */
int Mp4Writer::FinishMoovAtom()
{
	fMp4Stream.SetNextTrackId(fTrackCount + 1);
	fMp4Stream.SetCreationTime(fCreationTime);
	fMp4Stream.SetModificationTime(fModificationTime);
	fMp4Stream.EndMoovWriting();

	return 0;
}

/**
 * 完成写操作
 * 这个方法会结束写这个文件, 保存所有缓存的还没有写入的数据, 计算并
 * 写入相关的全局统计信息.
 * @param file 要写入的文件
 * @return 如果成功则返回 0. 
 */
int Mp4Writer::FinishTrack( Mp4TrackInfo* writeTrack, Mp4Track* track )
{	
	if (writeTrack == NULL || track == NULL) {
		return MP4_ERR_FAILED;
	}

	Mp4AtomPtr trackAtom = track->GetTrackAtom();
	if (trackAtom == NULL) {
		return MP4_ERR_FAILED;
	}

	// @todo 由于跳过了第一帧, 所以需要补上一帧的时间, 暂时填 0.
	INT64 duration = writeTrack->fDuration;
	if (duration <= 0) {
		UINT sampleCount = track->GetSampleCount();
		INT64 startTime = 0;
		INT64 sampleDuration = 0;
		if (sampleCount > 0) {
			track->GetSampleTimes(sampleCount - 1, &startTime, &sampleDuration);
			duration = startTime + sampleDuration;
			//LOG_D("%u, %u\r\n", (UINT)startTime, (UINT)duration);
		}
	}

	if (writeTrack->fSampleCount > 0) {
		UINT sampleTime = UINT((double)duration / writeTrack->fSampleCount + 0.5f);
		track->GetSampleTable().AddSampleTime(sampleTime);
	}

	track->SetMediaDuration(duration);
	track->SetTrackDuration(duration, GetTimeScale());
	track->Finish();

	//LOG_D("track: %u (%u)\r\n", (UINT)track->GetTrackDuration(), (UINT)GetTimeScale());

	// 计算 AAC 的相关统计信息
	Mp4AtomPtr mp4a = trackAtom->FindAtom("mdia.minf.stbl.stsd.mp4a");
	if (mp4a != NULL) {
		mp4a->SetPropertyInt("esds.bufferSize", track->GetMaxSampleSize());
		mp4a->SetPropertyInt("esds.avgBitrate", track->GetAvgBitrate());
		mp4a->SetPropertyInt("esds.maxBitrate", track->GetMaxBitrate());
	}

	return MP4_S_OK;
}

int Mp4Writer::Flush()
{

	return 0;
}


/** 返回这个文件的创建日期. */
INT64 Mp4Writer::GetCreationTime()
{
	return fCreationTime - 2082844800; // 1904 到 1970
}

/** 返回这个文件的总长度, 单位为毫秒. */
INT64 Mp4Writer::GetDuration()
{
	INT64 duration = fMp4Stream.ReadDuration();
	if (duration != 0) {
		return duration; // 换算成毫秒
	}

	if (fVideoTrackInfo) {
		duration = fVideoTrackInfo->fDuration;
		return Mp4Utils::Mp4TimeScale(duration, fVideoTrackInfo->fTimeScale, 1000); // 换算成毫秒
	}
	return 0;
}

/** 返回这个文件的总长度, 单位为字节. */
INT64 Mp4Writer::GetLength()
{
	return fMp4Stream.GetFileLength();
}

UINT Mp4Writer::GetTimeScale()
{
	return fMp4Stream.ReadTimeScale();
}

/** 指出是否打开了文件. */
BOOL Mp4Writer::IsOpen()
{
	return (fMp4Stream.GetFile() != NULL) && (fMp4Stream.GetMoovAtom() != NULL);
}

/** 
 * 创建指定名称的 MP4 文件. 
 * @param name 要创建的文件的名称.
 * @return 如果成功则返回 MP4_S_OK(0), 否则返回一个表示错误码的负数.
 *		- MP4_ERR_ALREADY_OPEN 文件已经打开了
 *		- MP4_ERR_OPEN 打开文件失败
 */
int Mp4Writer::Open( UINT flags )
{
	if (fMp4Stream.GetFile() != NULL) {
		return MP4_ERR_ALREADY_OPEN;	// 文件已经打开了

	} else if (Mp4IsEmpty(fFileName)) {
		return MP4_ERR_INVALID_PARAM;
	}

	char filename[MAX_PATH + 1];
	memset(filename, 0, sizeof(filename));
	int ret = 0;

	char* p = strrchr(fFileName, '.');
	if (p && strcmp(p, ".mdf") == 0) {
		*p = '\0';

		// Movie Data file
		Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mdf");
		Mp4FilePtr file = new Mp4File();
		if ((ret = file->Open(filename, "r+")) != MP4_S_OK) {
			file = NULL;	
			Close();
			return ret; // 文件打开失败
		}

		// Movie index file
		Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mif");
		fMetaWriter.Close();
		fMetaWriter.SetFileName(filename);
		fMetaWriter.LoadMetaInfo(this);

		// Movie stream
		fMp4Stream.OpenWrite(file, 1);

	} else {
		// Movie data file
		Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mdf");
		Mp4FilePtr file  = new Mp4File();
		if ((ret = file->Open(filename, "wb")) != MP4_S_OK) {
			file = NULL;	
			Close();
			return ret; // 文件打开失败
		}

		// Movie index file
		Mp4MergeExtName(filename, MAX_PATH, fFileName, ".mif");
		fMetaWriter.SetFileName(filename);
		if ((ret = fMetaWriter.Open(filename, 0)) != MP4_S_OK) {
			Close();
			return ret;
		}

		// Movie stream
		fMp4Stream.OpenWrite(file, 0);
		fMp4Stream.SetCreationTime(fCreationTime);
		fMp4Stream.SetModificationTime(fModificationTime);
	}

	return MP4_S_OK;
}

int Mp4Writer::Reset()
{
	// 初始化成员变量
	fAudioTrackInfo			= NULL;
	fCreationTime			= Mp4Utils::Mp4GetTimestamp();
	fDuration				= 0;
	fMetaWrited				= FALSE;
	fModificationTime		= fCreationTime;
	fMoiveFinished			= FALSE;
	fIsAACAudio				= FALSE;
	fTrackCount				= 0;
	fVideoIsSyncPoint		= TRUE;
	fVideoSyncPointCount	= 0;
	fVideoSampleSize		= 0;
	fVideoSampleSize		= 0;
	fVideoTrackInfo			= NULL;
	fWaitSyncPoint			= TRUE;

	return 0;
}

/**
 * 添加一个音频轨迹.
 * @param timeScale Time Scale
 * @param duration 
 * @param channels 音频通道数
 * @param type 音频编码类型
 * @param bitrate 码率
 * @return 返回这个 Track 的 ID. 
 */
int Mp4Writer::SetAudioFormat( UINT timeScale, INT64 duration, int channels, 
							  LPCSTR codecType, int param )
{
	if (codecType == NULL) {
		return 0;
	}

	if (fAudioTrackInfo == NULL) {
		fAudioTrackInfo = new Mp4TrackInfo();
		fTrackCount++;
	}

	Mp4TrackInfoPtr track = fAudioTrackInfo;
	track->fChannels	= channels;
	track->fCodecType	= Mp4Utils::AtomId(codecType);
	track->fParams		= param;
	track->fTimeScale	= timeScale;
	track->fTrackId		= fTrackCount;

	// chunk
	Mp4Chunk& chunk = track->GetChunk();
	chunk.SetMaxDuration(track->fTimeScale * 12 / 10);
	chunk.SetMaxSampleCount(47);

	return track->fTrackId;
}

void Mp4Writer::SetCreationTime( INT64 creationTime )
{
	fCreationTime = creationTime;
}

/** 添加 GPS 轨迹信息. */
int Mp4Writer::SetGpsTrack( LPCSTR track )
{
	if (track == NULL) {
		return 0;
	}

	Mp4AtomPtr atom = fMp4Stream.GetGpsTrackAtom(TRUE);
	if (atom) {
		atom->SetPropertyString("track", track);
	}
	return 0;
}

/** 设置输出文件名. */
int Mp4Writer::SetOutputFilename( LPCSTR filename )
{
	if (filename == NULL || *filename == '\0') {
		return -1;
	}

	strncpy(fFileName, filename, MAX_PATH);
	return 0;
}

/** 设置视频参数集. */
int Mp4Writer::SetVideoParamSets( char* ppsText, char* sqsText )
{
	if (fVideoTrackInfo == NULL) {
		return 0;
	}

	if (sqsText && fVideoTrackInfo->fSqsSets.GetCount() <= 0) {
		BYTE data[MAX_PATH];
		int size = Mp4Utils::HexDecode(sqsText, data, MAX_PATH);
		fVideoTrackInfo->fSqsSets.AddEntry(data, size);
	}

	if (ppsText && fVideoTrackInfo->fPpsSets.GetCount() <= 0) {
		BYTE data[MAX_PATH];
		int size = Mp4Utils::HexDecode(ppsText, data, MAX_PATH);
		fVideoTrackInfo->fPpsSets.AddEntry(data, size);
	}

	return 0;
}


/**
 * 添加一个视频 Track. 
 * @param timeScale Time scale
 * @param duration duration
 * @param width 视频宽度
 * @param height 视频高度
 * @param type 保留
 * @return 返回这个 Track 的 ID. 
 */
int Mp4Writer::SetVideoFormat( UINT timeScale, INT64 duration, WORD width, 
							  WORD height, LPCSTR codecType )
{
	if (fVideoTrackInfo == NULL) {
		fVideoTrackInfo = new Mp4TrackInfo();
		fTrackCount++;
	}

	Mp4TrackInfoPtr track = fVideoTrackInfo;
	track->fCodecType	= Mp4Utils::AtomId(codecType);
	track->fVideoHeight = height;
	track->fVideoWidth	= width;
	track->fTimeScale	= timeScale;
	track->fTrackId		= fTrackCount;

	Mp4Chunk& chunk = track->GetChunk();
	chunk.SetMaxDuration(track->fTimeScale * 12 / 10);
	chunk.SetMaxSampleCount(25);
	return track->fTrackId;
}

/**
 * 写音频 Sample.
 * @param data 这个 Sample 的内容
 * @param size 这个 Sample 的大小
 * @param timestamp 这个 Sample 的时间戳
 * @param timeScale 这个 Sample 的时间刻度
 * @return 如果成功则返回 MP4_S_OK. 
 */
int Mp4Writer::WriteAudioSample( MediaFileSample* sample)
{
	if (sample == NULL) {
		return MP4_ERR_FAILED;
	}

	BYTE* data		= sample->GetData();
	UINT  size		= sample->GetSize();
	INT64 timestamp	= sample->GetTimestamp();
	UINT  timeScale	= sample->GetTimeScale();
	if (data == NULL || size == 0) {
		return MP4_ERR_FAILED;

	} else if (fWaitSyncPoint) {
		//return MP4_S_OK;
	}

	Mp4TrackInfoPtr track = fAudioTrackInfo;
	if (track == NULL) {
		return MP4_ERR_FAILED;
	}

	if (fIsAACAudio) {
		// 去除 AAC ADTS 头 (7个字节)
		if ((*data == (BYTE)0xFF) && size > 7) {
			data += 7;
			size -= 7;
		}
	}

	// Add sample data
	track->GetChunk().PutSampleData(data, size);

	// Add sample
	MediaFileSamplePtr mediaSample = new MediaFileSample();
	mediaSample->SetTrackIndex(track->fTrackId);
	mediaSample->SetData(NULL);
	mediaSample->SetSize(size);
	mediaSample->SetMaxSize(size);
	mediaSample->SetDuration(track->GetDuration(timestamp, timeScale));
	mediaSample->SetSyncPoint(TRUE);
	return AddSample(track, mediaSample);
}

/**
 * 写入一个视频 Sample, 只支持 H.264.
 * @param sample 这个 Sample 的内容

 * @return 如果成功则返回 MP4_S_OK. 
 */
int Mp4Writer::WriteVideoSample( MediaFileSample* sample )
{
	if (sample == NULL) {
		return MP4_ERR_FAILED;
	}

	BYTE* sampleData = sample->GetData();
	UINT  sampleSize = sample->GetSize();
	BYTE nalHeader = 0;

	// 跳过 H.264 同步码, 一般为 "00 00 01" 或 "00 00 00 01"
	int startCodeLength = Mp4Video::GetStartCodeLength(sampleData);
	if (startCodeLength > 0) {
		sampleData	+= startCodeLength;
		sampleSize	-= startCodeLength;
		nalHeader	= sampleData[0];

		sample->SetData(sampleData);
		sample->SetSize(sampleSize);
	}
	
	return WriteVideoFragment(sample, nalHeader);
}

/**
 * 写入一个视频 Sample 分片, 主要用于 H.264.
 * @param sample 这个 Sample 分片的内容
 * @param nalHeader NAL 头, 如果这个分片不是这个 Sample 的第一片, 则为 0.
 * @return 如果成功则返回 MP4_S_OK. 
 */
int Mp4Writer::WriteVideoFragment( MediaFileSample* sample, BYTE nalHeader )
{
	if (sample == NULL) {
		return MP4_ERR_FAILED;
	}

	Mp4TrackInfoPtr track = fVideoTrackInfo;
	if (track == NULL) {
		return MP4_ERR_FAILED; // 不存在这个 track
	}

	// 检查是否等到了关键帧, 文件的第一帧视频必须是关键帧
	if (fWaitSyncPoint) {
		if (!sample->IsSyncPoint()) {
			return MP4_S_OK;
		}

		fWaitSyncPoint = FALSE;
	}

	if (sample->IsSyncPoint()) {
		fVideoIsSyncPoint = TRUE;
	}

	BYTE* data		= sample->GetData();
	UINT  size		= sample->GetSize();
	UINT  timeScale = sample->GetTimeScale();
	INT64 timestamp = sample->GetTimestamp();

	// 写入视频内容到缓存池中
	if (data && size > 0) {
		if (nalHeader) {
			FilterParamSets(data, size); // 提取序列/图像参数集等
		}

		Mp4Chunk& chunk = track->GetChunk();
		int ret = chunk.PutVideoFragment(data, size, nalHeader);
		if (ret <= 0) {
			return ret;
		}

		fVideoSampleSize += ret;
	}

	// 当前帧结束
	if (sample->IsEnd() || (fVideoSampleSize >= kMp4MaxFrameSize)) {
		BOOL isSyncPoint = fVideoIsSyncPoint;
		UINT sampleSize  = fVideoSampleSize;
		fVideoSampleSize = 0;

		if (fVideoIsSyncPoint) {
			fVideoIsSyncPoint = FALSE;
			fVideoSyncPointCount++;
		}

		UINT duration	= track->GetDuration(timestamp, timeScale);

		MediaFileSamplePtr mediaSample = new MediaFileSample();
		mediaSample->SetTrackIndex(track->fTrackId);
		mediaSample->SetData(NULL);
		mediaSample->SetMaxSize(sampleSize);
		mediaSample->SetSize(sampleSize);
		mediaSample->SetDuration(duration);
		mediaSample->SetSyncPoint(isSyncPoint);
		AddSample(track, mediaSample);
	}

	return size;
}

};
