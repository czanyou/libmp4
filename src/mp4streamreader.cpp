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
#include "mp4config.h"
#include "mp4file.h"
#include "mp4avcc.h"
#include "mp4streamreader.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4StreamReader

Mp4StreamReader* MediaCreateStreamReader()
{
	return new Mp4StreamReader();
}

Mp4StreamReader::Mp4StreamReader()
{
	fHeader		= NULL;
	fFileSize	= 0;
	fMdatOffset = 0;
	fPosition	= 0;
	fHeaderSize = 0;
}

Mp4StreamReader::~Mp4StreamReader()
{
	if (fHeader) {
		delete[] fHeader;
		fHeader = NULL;
	}
}

/** 设置新的读写位置. */
void Mp4StreamReader::SetPosition( INT64 position )
{
	fPosition = position;
	if (fMp4File == NULL) {
		return;
	}

	if (fPosition >= fHeaderSize) {
		fMp4File->SetPosition(fPosition - fHeaderSize + fMdatOffset);
	}
}

char* Mp4StreamReader::ReadTrackFile(LPCSTR filename)
{
	if (filename == NULL) {
		return NULL;
	}

	int len = strlen(filename);
	if (len < 4) {
		return NULL;
	}

	char path[MAX_PATH];
	memset(path, 0, sizeof(path));

	strncpy(path, filename, len - 4);
	strcat(path, ".gpl");

	printf("ReadTrackFile: %s\r\n", path);
	if (access(path, R_OK) != 0) {
		return NULL;
	}

	FILE* file = fopen(path, "rb");
	if (file == NULL) {
		return NULL;
	}

	struct stat s;
	if (fstat(fileno(file), &s) < 0) {
		return 0;
	}

	int fileSize = s.st_size;
	if (fileSize <= 0) {
		fclose(file);
		return NULL;
	}

	if (fileSize > 1024 * 64) {
		fileSize = 1024 * 64;
	}

	char* buffer = (char*)malloc(fileSize);
	if (buffer == NULL) {
		fclose(file);
		return NULL;
	}

	char* p = buffer;
	int leftover = fileSize;
	while (leftover > 0) {
		int size = fread(p, 1, 1024, file);
		if (size <= 0) {
			break;
		}

		p += size;
		leftover -= size;
	}

	printf("ReadTrackFile: %d\r\n", fileSize);

	fclose(file);
	file = NULL;

	return buffer;
}

/** 打开指定的 MP4 文件. */
int Mp4StreamReader::Open( LPCSTR filename )
{
	if (fHeader != NULL) {
		return MP4_ERR_ALREADY_OPEN;
	}

	Mp4ReaderPtr reader = new Mp4Reader();
	int ret = reader->Open(filename);
	if (ret != MP4_S_OK) {
		Close();
		return ret;
	}

	fMp4File = reader->GetFile();
	if (fMp4File == NULL) {
		Close();
		return MP4_ERR_NULL_FILE;
	}
	
	Mp4AtomPtr ftypAtom = reader->GetStream().GetFtypAtom();
	Mp4AtomPtr moovAtom = reader->GetStream().GetMoovAtom();
	if (ftypAtom == NULL || moovAtom == NULL) {
		Close();
		return MP4_ERR_NULL_ATOM;
	}

	char* track = ReadTrackFile(filename);
	if (track) {
		// printf("Open: %d\r\n", strlen(track));

		Mp4AtomPtr atom = moovAtom->GetChildAtom("gps ");
		if (atom == NULL) {
			atom = moovAtom->AddChildAtom("gps ");
		}

		if (atom) {
			atom->SetPropertyString("track", track);
		}
		free(track);
		track = NULL;
	}

	fHeaderSize = UINT(ftypAtom->CalculateSize() + moovAtom->CalculateSize());
	printf("Open: %d\r\n", fHeaderSize);

	reader->UpdateChunksOffset();
	if (!reader->GetStream().IsMoovLoaded()) {
		Close();
		return MP4_ERR_READ;

	} else if (fHeaderSize >= MP4_MAX_ATOM_SIZE) {
		Close();
		return MP4_ERR_ATOM_TOO_LARGE;
	}

	printf("Open2: %d\r\n", fHeaderSize);

	// Header Buffer
	fHeader = new BYTE[fHeaderSize + 512];
	Mp4MemFile file(fHeader, fHeaderSize + 512);

	ftypAtom->Write(&file);
	moovAtom->Write(&file);	

	char path[MAX_PATH + 1];
	memset(path, 0, sizeof(path));

	strncpy(path, filename, MAX_PATH);
	strncat(path, ".moov", MAX_PATH);

	fHeaderSize = (UINT)file.GetFileSize();

	// Mdat
	Mp4AtomPtr mdatAtom = reader->GetStream().GetMdatAtom();
	if (mdatAtom == NULL) {
		Close();
		return -3;
	}

	fFileSize   = fHeaderSize + mdatAtom->GetSize();
	fMdatOffset = mdatAtom->GetStart();
	fMp4File->SetPosition(fMdatOffset);

	// 清除缓存的内容
	reader->Clear();
	if (reader) {
		reader->Close();
		reader = NULL;
	}

	return 0;
}

/** 关闭这个文件. */
void Mp4StreamReader::Close()
{
	if (fMp4File) {
		fMp4File->Close();
		fMp4File = NULL;
	}

	if (fHeader) {
		delete[] fHeader;
		fHeader = NULL;
	}

	fHeader		= NULL;
	fFileSize	= 0;
	fMdatOffset = 0;
	fPosition	= 0;
	fHeaderSize = 0;
}

/** 读取指定长度的数据. */
UINT Mp4StreamReader::ReadBytes( BYTE* bytes, UINT numBytes )
{
	if (bytes == NULL || numBytes <= 0) {
		return 0;

	} else if (fMp4File == NULL || fHeader == NULL) {
		return 0;
	}

	// MP4 文件头
	if (fPosition < fHeaderSize) {
		UINT leftover = UINT(fHeaderSize - fPosition);
		UINT size = (leftover > numBytes) ? numBytes : leftover;
		memcpy(bytes, fHeader + fPosition, size);
		fPosition += size;

		// 文件头结束
		if (fPosition >= fHeaderSize) {
			fMp4File->SetPosition(fMdatOffset);
		}

		return size;

	// MP4 文件媒体数据内容
	} else {
		UINT size = fMp4File->ReadBytes(bytes, numBytes);
		fPosition += size;
		return size;
	}
}

};
