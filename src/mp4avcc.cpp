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
#include "mp4avcc.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4AvcConfig class

/**
 * 构建一个新的 Mp4AvcConfig
 * @param avcC AvcC Atom
 */
Mp4AvcConfig::Mp4AvcConfig( Mp4Atom* avcC )
{
	fAvcC = avcC;
}

/**
 * 添加一个 H.264 图像参数集. 
 * @param pictureSets 
 * @param length 
 */
BOOL Mp4AvcConfig::AddPictureParameters(const BYTE* pictureSets, WORD length)
{
	if (fAvcC == NULL) {
		return FALSE;
	}

	Mp4PropertyPtr count = fAvcC->FindProperty("numOfPictureParameterSets");
	Mp4SizeTablePropertyPtr table = GetSizeTable("pictureEntries");
	if (count != NULL && table != NULL) {
		table->AddEntry(pictureSets, length);
		count->SetValueInt(table->GetCount());
		return TRUE;
	}
	return FALSE;
}

/**
 * 添加一个 H.264 序列参数集.
 * @param sequenceSets 
 * @param length 
 */
BOOL Mp4AvcConfig::AddSequenceParameters(const BYTE* sequenceSets, WORD length)
{
	if (fAvcC == NULL) {
		return FALSE;
	}

	Mp4PropertyPtr count = fAvcC->FindProperty("numOfSequenceParameterSets");
	Mp4SizeTablePropertyPtr table = GetSizeTable("sequenceEntries");
	if (count != NULL && table != NULL) {
		table->AddEntry(sequenceSets, length);
		count->SetValueInt(table->GetCount());
		return TRUE;
	}
	return FALSE;
}

/** 返回长度. */
UINT Mp4AvcConfig::GetLengthSize ()
{
	return fAvcC ? (UINT)(fAvcC->GetPropertyInt("lengthSizeMinusOne") & 0x03) : 0;
}

/** 返回 H.264 图像参数集的数目. */
UINT Mp4AvcConfig::GetPictureSetCount()
{
	return fAvcC ? (UINT)fAvcC->GetPropertyInt("numOfPictureParameterSets") : 0;
}

/** 
 * 返回指定的索引的 H.264 图像参数集的内容和长度. 
 * @param index 索引值
 * @param length [out] 返回指定的参数集的长度.
 * @return 返回指向指定的参数集数据的指针, 如果不存在则返回 NULL.
 */
BYTE* Mp4AvcConfig::GetPictureParameters(int index, WORD& length)
{	
	Mp4SizeTablePropertyPtr table = GetSizeTable("pictureEntries");
	return table ? table->GetEntry(index, length) : NULL;
}

/** 返回 H.264 Profile 和级别参数的值. */
void Mp4AvcConfig::GetProfileLevel (BYTE *profile, BYTE *level)
{
	if (fAvcC == NULL) {
		return;
	}

	if (profile != NULL) {
		*profile = (BYTE)fAvcC->GetPropertyInt("AVCProfileIndication");
	}

	if (level != NULL) {
		*level	  = (BYTE)fAvcC->GetPropertyInt("AVCLevelIndication");
	}
}

/**
 * 返回 profile_compatibility 属性的值.
 * @return 这个属性的值. 
 */
BYTE Mp4AvcConfig::GetProfileCompatibility()
{
	return fAvcC ? (BYTE)fAvcC->GetPropertyInt("profile_compatibility") : 0;
}

/** 返回 H.264 序列参数集的数目. */
UINT Mp4AvcConfig::GetSequenceSetCount()
{
	return fAvcC ? (UINT)fAvcC->GetPropertyInt("numOfSequenceParameterSets") : 0;
}

/** 
 * 返回指定的索引的 H.264 序列参数集的内容和长度. 
 * @param index 索引值
 * @param length [out] 返回指定的参数集的长度.
 * @return 返回指向指定的参数集数据的指针, 如果不存在则返回 NULL.
 */
BYTE* Mp4AvcConfig::GetSequenceParameters(int index, WORD& length)
{
	Mp4SizeTablePropertyPtr table = GetSizeTable("sequenceEntries");
	return table ? table->GetEntry(index, length) : NULL;
}

/** 返回指定的名称的 SizeTable 属性. */
Mp4SizeTablePropertyPtr Mp4AvcConfig::GetSizeTable(LPCSTR name)
{
	Mp4PropertyPtr property = fAvcC ? fAvcC->FindProperty(name) : NULL;
	if (property == NULL || property->GetType() != PT_SIZE_TABLE) {
		return NULL;
	}
	return reinterpret_cast<Mp4SizeTableProperty*>(property.ptr);
}

/**
 * 设置 profile_compatibility 属性的值.
 * @param compatibility 
 */
void Mp4AvcConfig::SetProfileCompatibility( BYTE compatibility )
{
	if (fAvcC) {
		fAvcC->SetPropertyInt("profile_compatibility", compatibility);
	}
}

/** 设置 H.264 Profile 和级别参数的值. */
void Mp4AvcConfig::SetProfileLevel (BYTE profile, BYTE level)
{
	if (fAvcC) {
		fAvcC->SetPropertyInt("AVCProfileIndication", profile);
		fAvcC->SetPropertyInt("AVCLevelIndication", level);
	}
}

};

