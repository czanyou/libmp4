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
#ifndef _NS_VISION_MP4_PROPERTY_H
#define _NS_VISION_MP4_PROPERTY_H

#include "time.h"
#include "mp4config.h"
#include "mp4file.h"
#include "mp4common.h"

namespace mp4 {

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4PropertyType

/** MP4 Atom 属性类型. */
enum Mp4PropertyType
{
	PT_INTEGER			= 1,	///< 8, 16, 24, 32, 或 64 位整数类型属性. 
	PT_FLOAT			= 2,	///< 浮点类型属性
	PT_BITS				= 3,	///< 比特类型属性
	PT_BYTES			= 4,	///< 二进制数据类型属性
	PT_STRING			= 5,	///< 字符串类型属性
	PT_TABLE			= 6,	///< 表格类型属性
	PT_DESCRIPTOR		= 7,	///< Descriptor 类型属性
	PT_INT_ARRAY		= 8,	///< 整数数组类型属性
	PT_SIZE_TABLE		= 9,	///< 整数数组类型属性
	PT_COUNT			= 10
};

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Property

class Mp4File;
class Mp4Property;

/** Mp4Property 智能指针类型. */
typedef SmartPtr<Mp4Property> Mp4PropertyPtr;

/** Mp4Property 智能指针类型数组. */
typedef Mp4Array<Mp4PropertyPtr> Mp4PropertyArray;

/** 
 * 代表一个 MP4 atom 的基本属性. 
 * 一个 MP4 Atom 通常是由一组属性和子 Atom 组成的.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Property : public VisionBaseObject
{
public:
	Mp4Property(Mp4PropertyType type, UINT size = 0, LPCSTR name = NULL);
	virtual ~Mp4Property();
	VISION_BASEREF_METHODS(Mp4Property);	///< 实现引用计数功能

// Attributes -------------------------------------------------
public:
	Mp4PropertyType GetType();
	LPCSTR GetBytes() const;
	LPCSTR GetName() const;
	UINT   GetSize();
	UINT   GetValueString(char* buf, UINT bufLen);
	float  GetValueFloat();

	void   SetValueInt(INT64 value);
	void   SetValueFloat(float value);
	void   SetValueBytes(const BYTE* bytes, UINT count);
	void   SetValueString(LPCSTR value);

// Operations -------------------------------------------------
public:
	virtual UINT  CalculateSize();
	virtual INT64 GetValueInt();
	virtual void  SetExpectSize(UINT count);
	virtual int   Read    (Mp4File* file);
	virtual int   Write   (Mp4File* file);

private:
	int ReadValueFloat  (Mp4File* file);
	int ReadValueString (Mp4File* file);
	int ReadValueBytes  (Mp4File* file);
	int WriteValueFloat (Mp4File* file);
	int WriteValueString(Mp4File* file);

// Data Members -----------------------------------------------
protected:
	Mp4PropertyType fType;	///< 属性的类型
	char* fName;			///< 属性名称
	UINT  fSize;			///< 值的长度.
	UINT  fExpectSize;		///< 期望的大小或长度

	INT64 fValueInt;		///< 整型值
	float fValueFloat;		///< 浮点型值
	char* fValueBytes;		///< 字符串/字节/其他类型值
};

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Descriptor

class Mp4Descriptor;

/** Mp4Descriptor 智能指针类型. */
typedef SmartPtr<Mp4Descriptor> Mp4DescriptorPtr;

/** Mp4Descriptor 智能指针类型数组. */
typedef Mp4Array<Mp4DescriptorPtr> Mp4DescriptorArray;

/** 
 * 代表一个 MP4 Descriptor 对象. 主要用于 "iods,esds" atom 节点. 
 * 
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Descriptor : public VisionBaseObject
{
public:
	Mp4Descriptor(Mp4DescriptorType type);
	VISION_BASEREF_METHODS(Mp4Descriptor);		///< 实现引用计数功能

	enum Config {
		MP4_MPEG4_AUDIO_TYPE	= 0x40,
		MP4_AUDIOSTREAMTYPE		= 0x15 
	};

// Attributes -------------------------------------------------
public:
	Mp4PropertyPtr GetProperty(LPCSTR name);
	UINT GetPropertyValue(LPCSTR name);
	BYTE GetSize()				{ return fSize; }	///< 返回这个对象的大小
	Mp4DescriptorType GetType() { return fType; }	///< 返回这个对象的类型

	void SetSize(BYTE size) { fSize = size; }		///< 设置这个对象的大小

// Operations -------------------------------------------------
public:
	void Build();

	int Read(Mp4File* file);
	int Write(Mp4File* file);

	void WriteMpegLength(UINT length, UINT& count);
	void WriteInt(UINT value, UINT size, UINT& count);

// Data Members -----------------------------------------------
private:
	BYTE fSize;							///< 这个 Descriptor 的数据的长度
	BYTE fBuffer[MAX_PATH];				///< 这个 Descriptor 的数据缓存区
	
	Mp4DescriptorType  fType;			///< 这个 Descriptor 的类型
	Mp4PropertyArray   fProperties;		///< 这个 Descriptor 的属性列表
	Mp4DescriptorArray fDescriptors;	///< 子 Descriptor 列表.
};


//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4DescriptorProperty class

/** 
 * 代表一个 Descriptor 类型的属性. 
 * 
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4DescriptorProperty : public Mp4Property
{
public:
	Mp4DescriptorProperty(LPCSTR name);
	~Mp4DescriptorProperty();

// Attributes -------------------------------------------------
public:
	UINT  GetDescriptorCount();
	INT64 GetIntValue();

// Operations -------------------------------------------------
public:
	int Read(Mp4File* file);
	int Write(Mp4File* file);

	Mp4PropertyPtr GetProperty(LPCSTR name);
	Mp4DescriptorPtr GetDescriptor(UINT index);
	void AddDescriptor(Mp4Descriptor* descriptor);

	UINT CalculateSize();

// Data Members -----------------------------------------------
private:
	Mp4DescriptorArray fDescriptors;	///< 子 Descriptor 列表
	BYTE* fBuffer;
	UINT  fBufferSize;
};

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4SizeTableProperty

/** 
 * 代表一个 MP4 字节数组表, 仅用于 AvcC atom 节点. 
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4SizeTableProperty : public Mp4Property
{
public:
	Mp4SizeTableProperty();
	Mp4SizeTableProperty(LPCSTR name);
	virtual ~Mp4SizeTableProperty();

// Attributes -------------------------------------------------
public:
	UINT  GetCount();

// Operations -------------------------------------------------
public:
	void  AddEntry(const BYTE* bytes, WORD length);
	BYTE* GetEntry(UINT index, WORD& length);

	void Clear();
	UINT CalculateSize();

	int  Read(Mp4File* file);
	int  Write(Mp4File* file);

// Data Members -----------------------------------------------
private:
	Mp4Array<WORD>  fSizeArray;		///< 字节数组长度表
	Mp4Array<BYTE*> fBytesArray;	///< 字节数组表
};

/** Mp4SizeTableProperty 智能指针类型. */
typedef SmartPtr<Mp4SizeTableProperty> Mp4SizeTablePropertyPtr;

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4TableProperty

/** 
 * 代表一个 MP4 表格属性. 
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4TableProperty : public Mp4Property
{
public:
	Mp4TableProperty(LPCSTR name = NULL);

// Attributes -------------------------------------------------
public:
	UINT   GetColumnCount();
	LPCSTR GetColumnName(UINT colum);
	UINT   GetRowCount();
	BOOL   IsEmpty();

// Operations -------------------------------------------------
public:
	void AddColumn(LPCSTR name);
	void AddRow(UINT value0, UINT value1 = 0, UINT value2 = 0, UINT value3 = 0);
	UINT CalculateSize();
	int  Read (Mp4File* file);
	UINT GetValue(UINT row, UINT colum = 0);
	UINT Search(UINT value, UINT colum = 0);
	void SetValue(UINT row, UINT value, UINT colum = 0);
	int  Write(Mp4File* file);

// Data Members -----------------------------------------------
private:
	Mp4PropertyArray fColumnNames;	///< 属性列表.
	Mp4Array<UINT> fTable;			///< 数组属性
	UINT fColumnCount;				///< 
	UINT fRowCount;					///< 
};

/** Mp4TableProperty 智能指针类型. */
typedef SmartPtr<Mp4TableProperty> Mp4TablePropertyPtr;

};

#endif // !defined(_NS_VISION_MP4_PROPERTY_H)
