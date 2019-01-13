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
#ifndef _NS_VISION_MP4_STREAM_HHH_
#define _NS_VISION_MP4_STREAM_HHH_

#include "mp4common.h"
#include "mp4atom.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Stream

/** 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Stream
{
public:
	Mp4Stream();
	~Mp4Stream();

// Attributes -------------------------------------------------
public:
	Mp4FilePtr GetFile();
	Mp4AtomPtr GetCurrentAtom();
	Mp4AtomPtr GetFtypAtom();
	Mp4AtomPtr GetMoovAtom();
	Mp4AtomPtr GetMdatAtom();
	Mp4AtomPtr GetRootAtom();
	INT64 GetChunksOffset();
	INT64 GetReadPosition();
	INT64 GetFileLength();
	BOOL  IsMoovLoaded();
	void  SetCreationTime(INT64 time);
	void  SetDuration( INT64 duration );
	void  SetFile( Mp4File* file );
	void  SetModificationTime(INT64 time);
	void  SetNextTrackId(UINT trackId);

// Operations -------------------------------------------------
public:
	Mp4AtomPtr AddTrackAtom();
	Mp4AtomPtr GetGpsTrackAtom(BOOL add = FALSE);
	int   BeginWriting();
	void  Close();
	void  Clear();
	int   EndMdatWriting();
	int   EndMoovWriting();

	int   LoadMoovAtom();
	int   LoadMvhdAtom();
	int   LoadGpsAtom();
	int   LoadTopAtoms(Mp4File* file);

	int   OpenRead( Mp4File* file, UINT flags );
	int   OpenWrite( Mp4File* file, UINT flags );

	INT64 ReadCreationTime();
	INT64 ReadDuration();
	UINT  ReadTimeScale();

private:
	int   ReadAtomHeader( Mp4File* file, INT64 fileSize, Mp4AtomPtr& atom );
	int   ReadFtypAtom( Mp4File* file, INT64 fileSize );
	int   ReadTopAtoms();

// Data Members -----------------------------------------------
private:
	Mp4AtomPtr fCurrentAtom;	///< 当前正在读取的顶级 ATOM
	Mp4AtomPtr fFtypAtom;		///< ftyp Atom
	Mp4AtomPtr fMdatAtom;		///< mdat Atom
	Mp4AtomPtr fMoovAtom;		///< moov Atom
	Mp4AtomPtr fGpstAtom;		///< gpst Atom
	Mp4AtomPtr fRootAtom;		///< root Atom
	Mp4FilePtr fMp4File;		///< MP4 文件


	INT64	fReadPosition;		///< 当前读取的位置, 单位为字节
};

};

#endif // !defined(_NS_VISION_MP4_STREAM_HHH_)
