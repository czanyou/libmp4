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
#include "mp4stream.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Stream

Mp4Stream::Mp4Stream()
{
	fReadPosition	= 0;
}

Mp4Stream::~Mp4Stream()
{
	Close();
}

Mp4AtomPtr Mp4Stream::AddTrackAtom()
{
	Mp4AtomPtr moovAtom = GetMoovAtom();
	if (moovAtom == NULL) {
		return NULL;
	} 

	// 创建 trak 节点
	return moovAtom->AddChildAtom("trak");
}

int Mp4Stream::BeginWriting()
{
	Mp4AtomPtr ftypAtom = fFtypAtom;
	Mp4AtomPtr mdatAtom = fMdatAtom;

	if (ftypAtom == NULL || mdatAtom == NULL) {
		return MP4_ERR_NOT_OPEN;
	}

	// 数据文件头
	Mp4FilePtr file = fMp4File;
	if (file) {
		file->SetPosition(0);
		ftypAtom->Write(file);
		mdatAtom->WriteHeader(file);
	}

	return MP4_S_OK;
}

void Mp4Stream::Clear()
{
	if (fMp4File) {
		fMp4File = NULL;
	}

	if (fCurrentAtom) {
		fCurrentAtom->Clear();
		fCurrentAtom = NULL;
	}

	if (fFtypAtom) {
		fFtypAtom->Clear();
		fFtypAtom = NULL;
	}

	if (fMoovAtom) {
		fMoovAtom->Clear();
		fMoovAtom = NULL;
	}

	if (fMdatAtom) {
		fMdatAtom->Clear();
		fMdatAtom = NULL;
	}

	if (fRootAtom) {
		fRootAtom->Clear();
		fRootAtom = NULL;
	}

	fReadPosition = 0;
}

void Mp4Stream::Close()
{
	if (fMp4File) {
		fMp4File->Close();
	}

	Clear();
}

int Mp4Stream::EndMdatWriting()
{
	Mp4FilePtr file = GetFile();
	Mp4AtomPtr mdatAtom = GetMdatAtom();
	if (mdatAtom) {
		mdatAtom->WriteSize(file);
	}

	return 0;
}

int Mp4Stream::EndMoovWriting()
{
	Mp4AtomPtr moovAtom = GetMoovAtom();
	if (moovAtom == NULL) {
		return 0;
	}

	if (fGpstAtom) {
		moovAtom->RemoveChildAtom("gps ");
		moovAtom->AddChildAtom(fGpstAtom);
	}

	moovAtom->Write(GetFile());
	return 0;
}

INT64 Mp4Stream::GetChunksOffset()
{
	if (!IsMoovLoaded()) {
		return 0;
	}

	// 计算 MDAT 要偏移的长度
	INT64 offset = fFtypAtom->CalculateSize();
	offset += fMoovAtom->CalculateSize();

	offset -= fMdatAtom->GetStart();
	return offset;
}

Mp4AtomPtr Mp4Stream::GetCurrentAtom()
{
	return fCurrentAtom;
}

/** 返回相关的 Mp4File 对象. */
Mp4FilePtr Mp4Stream::GetFile()
{
	return fMp4File;
}

/** 返回这个文件的总长度, 单位为字节. */
INT64 Mp4Stream::GetFileLength()
{
	if (fMp4File) {
		return fMp4File->GetFileSize();
	}

	return 0;
}

Mp4AtomPtr Mp4Stream::GetFtypAtom()
{
	return fFtypAtom;
}

Mp4AtomPtr Mp4Stream::GetGpsTrackAtom( BOOL add /*= FALSE*/ )
{
	if (fGpstAtom) {
		return fGpstAtom;
	}

	Mp4AtomPtr moov = GetMoovAtom();
	if (moov) {
		fGpstAtom = moov->GetChildAtom("gps ");
		if (fGpstAtom) {
			return fGpstAtom;
		}
	}

	fGpstAtom = new Mp4Atom("gps ");
	fGpstAtom->Init(0);
	return fGpstAtom;
}

Mp4AtomPtr Mp4Stream::GetMdatAtom()
{
	return fMdatAtom;
}

Mp4AtomPtr Mp4Stream::GetMoovAtom()
{
	return fMoovAtom;
}

INT64 Mp4Stream::GetReadPosition()
{
	return fReadPosition;
}

Mp4AtomPtr Mp4Stream::GetRootAtom()
{
	return fRootAtom;
}

BOOL Mp4Stream::IsMoovLoaded()
{
	if (fMp4File == NULL) {
		return FALSE;

	} else if (fFtypAtom == NULL) {
		return FALSE;

	} else if (fMoovAtom == NULL) {
		return FALSE;

	} else if (fMdatAtom == NULL) {
		return FALSE;
	}

	return TRUE;
}

int Mp4Stream::LoadMoovAtom()
{
	Mp4FilePtr file = fMp4File;

	if (fMoovAtom == NULL || file == NULL) {
		return MP4_ERR_FAILED;
	}

	// 创建根节点并读取所有的子节点
	file->SetPosition(fMoovAtom->GetStart() + MP4_ATOM_HEADER_SIZE);
	fMoovAtom->Read(file);

	return 0;
}

int Mp4Stream::LoadMvhdAtom()
{
	Mp4FilePtr file = fMp4File;
	if (fMoovAtom == NULL || file == NULL) {
		return MP4_ERR_FAILED;
	}

	// 创建根节点并读取所有的子节点
	file->SetPosition(fMoovAtom->GetStart() + MP4_ATOM_HEADER_SIZE);
	fMoovAtom->Read(file, 1);

	Mp4AtomPtr atom = fMoovAtom->GetChildAtom("mvhd");
	if (atom) {
		file->SetPosition(atom->GetStart() + MP4_ATOM_HEADER_SIZE);
		atom->Read(file, 1);
	}

	return 0;
}

int Mp4Stream::LoadGpsAtom()
{
	Mp4FilePtr file = fMp4File;
	if (fMoovAtom == NULL || file == NULL) {
		return MP4_ERR_FAILED;
	}

	// 创建根节点并读取所有的子节点
	file->SetPosition(fMoovAtom->GetStart() + MP4_ATOM_HEADER_SIZE);
	fMoovAtom->Read(file, 1);

	Mp4AtomPtr atom = fMoovAtom->GetChildAtom("gps ");
	if (atom) {
		file->SetPosition(atom->GetStart() + MP4_ATOM_HEADER_SIZE);
		atom->Read(file, 1);
	}

	return 0;
}

/** 
 * 探测 MP4 文件的顶级 Atoms. 
 */
int Mp4Stream::LoadTopAtoms(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_NOT_OPEN;
	}

	INT64 fileSize = file->GetFileSize();
	if (fileSize <= MP4_ATOM_HEADER_SIZE) {
		return 0; // 文件太小, 接近为空
	}

	if (fRootAtom == NULL) {
		fRootAtom = new Mp4Atom("root");
	}

	// 检查第一个 ATOM 是否为 ftyp.
	if ((fReadPosition == 0) && (fFtypAtom == NULL)) {
		int ret = ReadFtypAtom(file, fileSize);
		if (ret <= 0) {
			return ret;
		}

		fRootAtom->AddChildAtom(fFtypAtom);
	}

	while (1) {

		// 读取下一个顶级 ATOM 的头信息
		if (fCurrentAtom == NULL) {
			file->SetPosition(fReadPosition);
			int ret = ReadAtomHeader(file, fileSize, fCurrentAtom);
			if (ret <= 0 || fCurrentAtom == NULL) {
				return ret;
			}
		
			INT64 atomSize = fCurrentAtom->GetSize();
			if (atomSize < MP4_ATOM_HEADER_SIZE) {
				return MP4_ERR_FAILED;
			}

			UINT atomType = Mp4Utils::AtomId(fCurrentAtom->GetType());
			if (atomType == Mp4Utils::AtomId("mdat")) {
				fMdatAtom = fCurrentAtom;
			}
		}

		// 检查这个 Atom 是否完整
		INT64 end = fCurrentAtom->GetEnd();
		if (fileSize < end) {
			break;
		}

		UINT atomType = Mp4Utils::AtomId(fCurrentAtom->GetType());
		if (atomType == Mp4Utils::AtomId("moov")) {
			fMoovAtom = fCurrentAtom;
		}

		fRootAtom->AddChildAtom(fCurrentAtom);

		// 跳到下一个顶级 ATOM 的开始位置
		fReadPosition = end;
		fCurrentAtom = NULL;
	}

	return 0;
}

/**
* 打开指定的 MP4 文件. 
* @param file 
* @return 如果成功, 则返回 MP4_S_OK.
*/
int Mp4Stream::OpenRead( Mp4File* file, UINT flags )
{
	if (fMp4File != NULL || file == NULL) {
		return MP4_ERR_ALREADY_OPEN;
	}

	fMp4File		= file;
	fReadPosition	= 0;

	int ret = LoadTopAtoms(file);
	if (ret < 0) {
		Close();
	}

	return ret;
}

int Mp4Stream::OpenWrite( Mp4File* file, UINT flags )
{
	fMp4File = file;

	if (flags == 1) {
		ReadTopAtoms();

		// Moov Atom
		if (fMoovAtom == NULL) {
			fMoovAtom = new Mp4Atom("moov");
		}
		fMoovAtom->ClearChildAtoms();
		fMoovAtom->Init(0);

	} else if (flags == 2) {
		ReadTopAtoms();

	} else {
		// 创建根 ATOM 节点以及必要的子节点
		fFtypAtom = new Mp4Atom("ftyp");
		fFtypAtom->Init(0);

		// moov atom
		fMoovAtom = new Mp4Atom("moov");
		fMoovAtom->Init(0);

		// mdat atom
		fMdatAtom = new Mp4Atom("mdat");
	}

	return 0;
}

int Mp4Stream::ReadAtomHeader( Mp4File* file, INT64 fileSize, Mp4AtomPtr& atom )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	INT64 start = file->GetPosition();
	if ((fileSize - start) < MP4_ATOM_HEADER_SIZE) {
		return 0; // ATOM header 至少 8 个字节
	}

	// 读取 ATOM header
	char type[5] = {0, 0, 0, 0, 0};
	INT64 atomSize = file->ReadInt(4);
	file->ReadBytes((BYTE*)type, 4);

	atom = new Mp4Atom(type);
	atom->SetStart(start);
	atom->SetSize(atomSize);

	return MP4_ATOM_HEADER_SIZE;
}

/** 返回文件的创建时间, 1970-1-1 以来经过的秒数. */
INT64 Mp4Stream::ReadCreationTime()
{
	if (fMoovAtom) {
		INT64 creationTime = fMoovAtom->GetPropertyInt("mvhd.creationTime");
		if (creationTime > 0) {
			return creationTime - 2082844800; // 1904 - 1970
		}
	}
	return 0;
}

/** 返回指定的 track 的长度, 单位为毫秒. */
INT64 Mp4Stream::ReadDuration()
{
	if (fMoovAtom ) {
		INT64 duration = fMoovAtom->GetPropertyInt("mvhd.duration");
		return Mp4Utils::Mp4TimeScale(duration, ReadTimeScale(), 1000);
	}

	return 0;
}

int Mp4Stream::ReadFtypAtom( Mp4File* file, INT64 fileSize )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	file->SetPosition(0);
	Mp4AtomPtr ftyp;
	int ret = ReadAtomHeader(file, fileSize, ftyp);
	if (ret <= 0 || ftyp == NULL) {
		return ret;
	}

	// 检查第一个 Atom 是否为 ftyp.
	LPCSTR type = ftyp->GetType();
	if (Mp4Utils::AtomId(type) != Mp4Utils::AtomId("ftyp")) {
		return MP4_ERR_FAILED;
	}

	INT64 end = ftyp->GetEnd();
	if (end > fileSize) {
		return 0;
	}

	// Read ftyp Content
	ret = ftyp->Read(file);
	if (ret != MP4_S_OK) {
		return ret;
	}

	fFtypAtom		= ftyp;
	fReadPosition	= end;

	return (int)ftyp->GetSize();
}

/** 返回文件的时间比. */
UINT Mp4Stream::ReadTimeScale()
{
	if (fMoovAtom) {
		return fMoovAtom->GetPropertyInt("mvhd.timeScale");
	}
	return 0;
}

/** 探测 MP4 文件的顶级 Atoms. */
int Mp4Stream::ReadTopAtoms()
{
	Mp4FilePtr file = fMp4File;
	if (file == NULL) {
		return MP4_ERR_NOT_OPEN;
	}

	INT64 fileSize = file->GetFileSize();
	if (fileSize <= MP4_ATOM_HEADER_SIZE) {
		return 0;
	}

	// ftyp Atom
	int ret = ReadFtypAtom(file, fileSize);
	if (ret <= 0) {
		return ret;
	}

	INT64 position = fFtypAtom->GetEnd();

	while (TRUE) {
		file->SetPosition(position);

		// 读取下一个顶级 ATOM
		Mp4AtomPtr atom;
		int ret = ReadAtomHeader(file, fileSize, atom);
		if (ret <= 0 || atom == NULL) {
			break;
		}

		UINT atomType = Mp4Utils::AtomId(atom->GetType());
		if (atomType == Mp4Utils::AtomId("mdat")) {
			fMdatAtom = atom;

		} else if (atomType == Mp4Utils::AtomId("moov")) {
			fMoovAtom = atom;
		}

		if (atom->GetSize() <= 0) {
			break;
		}

		// 检查这个 ATOM 是否完整
		INT64 end = atom->GetEnd();
		if (fileSize < end) {
			break;
		}

		// 跳到下一个顶级 ATOM 的开始位置
		position = end;
	}

	// 跳到 moov Atom 开始位置
	position = fMoovAtom ? fMoovAtom->GetStart() : fileSize;
	file->SetPosition(position);

	if (fMdatAtom && fMdatAtom->GetSize() <= 0) {
		INT64 size = position - fMdatAtom->GetStart();
		fMdatAtom->SetSize(size);
	}

	return 0;
}

void Mp4Stream::SetDuration( INT64 duration )
{
	if (fMoovAtom) {
		fMoovAtom->SetPropertyInt("mvhd.duration", duration);
	}
}

void Mp4Stream::SetCreationTime( INT64 time )
{
	if (fMoovAtom) {
		fMoovAtom->SetPropertyInt("mvhd.creationTime", time);
	}
}

void Mp4Stream::SetFile( Mp4File* file )
{
	fMp4File = file;
}

void Mp4Stream::SetModificationTime( INT64 time )
{
	if (fMoovAtom) {
		fMoovAtom->SetPropertyInt("mvhd.modificationTime", time);
	}
}

void Mp4Stream::SetNextTrackId( UINT trackId )
{
	if (fMoovAtom) {
		fMoovAtom->SetPropertyInt("mvhd.nextTrackId", trackId);
	}
}

}
