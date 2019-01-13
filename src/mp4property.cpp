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

#include <sys/stat.h>
#include "mp4config.h"
#include "mp4property.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Property class

/** 构建一个新的属性. */
Mp4Property::Mp4Property( Mp4PropertyType type, UINT size, LPCSTR name)
{
	fExpectSize		= 0;
	fName			= NULL;
	fSize			= size;
	fType			= type;
	fValueInt		= 0;
	fValueFloat		= 0;
	fValueBytes		= NULL;

	if (name) {
		fName = new char[strlen(name) + 1];
		strcpy(fName, name);
	} 

	if (type == PT_STRING || type == PT_BYTES) {
		if (size > 0) {
			fValueBytes = new char[size + 1];
			memset(fValueBytes, 0, size + 1);
		}
	}
}

Mp4Property::~Mp4Property()
{
	if (fValueBytes) {
		delete[] fValueBytes;
		fValueBytes = NULL;
	}

	if (fName) {
		delete[] fName;
		fName = NULL;
	}
}

/** 返回当前属性的字节/字符串的值. */
LPCSTR Mp4Property::GetBytes() const
{
	if (fValueBytes == NULL && fType == PT_STRING) {
		return "                ";
	}
	return fValueBytes;
}

/** 返回这个属性的名称. */
LPCSTR Mp4Property::GetName() const
{
	return fName ? fName : "";
}

/** 返回这个属性所占用的空间, 单位为字节. 返回 0 表示不确定. */
UINT Mp4Property::GetSize()
{
	return fSize;
}

/** 返回这个属性的类型. */
Mp4PropertyType Mp4Property::GetType()
{
	return fType;
}

/** 返回当前属性的整数值. */
INT64 Mp4Property::GetValueInt()
{
	if (fType == PT_FLOAT) {
		return (INT64)fValueFloat;
	} else {
		return fValueInt;
	}
}

/** 返回当前属性的浮点型值. */
float Mp4Property::GetValueFloat()
{
	if (fType == PT_INTEGER) {
		return (float)fValueInt;
	} else {
		return fValueFloat;
	}
}

/** 返回当前属性的字符串的值. */
UINT Mp4Property::GetValueString( char* buf, UINT bufLen )
{
	if (buf == NULL || bufLen <= 1) {
		return 0;
	}

	if (fValueBytes) {
		UINT size = (fSize < (bufLen - 1)) ? fSize : (bufLen - 1);
		strncpy(buf, fValueBytes, size);
		return size;
	}
	return 0;
}

/**
 * 从指定文件的当前位置读取这个属性的内容.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::Read( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	switch (fType) {
	case PT_INTEGER:	SetValueInt(file->ReadInt64(fSize));	break;
	case PT_BITS:		SetValueInt(file->ReadBits(fSize));		break;
	case PT_BYTES:		ReadValueBytes(file);					break;
	case PT_FLOAT:		ReadValueFloat(file);					break;
	case PT_STRING:		ReadValueString(file);					break;
	case PT_TABLE:
	case PT_DESCRIPTOR:
	case PT_INT_ARRAY:
	case PT_SIZE_TABLE:
		// 在子类中处理
		break;
	case PT_COUNT:
		break;
	}

	return MP4_S_OK;
}

/**
 * 从指定的文件当前位置读取指定个字节.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::ReadValueBytes(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	UINT size = fSize;
	if (size <= 0) {
		size = fExpectSize;		// 如果没有指定固定长度, 则使用期望长度
		if (size <= 0) {
			return MP4_ERR_INVALID_LENGTH;
		}

		fSize = size;
	}

	if (size >= 64 * 1024) {
		return MP4_ERR_FAILED;
	}

	BYTE* data = new BYTE[size + 1];
	UINT ret = file->ReadBytes((BYTE*)data, size);
	if (ret > 0) {
		SetValueBytes(data, ret);
	}
	delete[] data;
	return MP4_S_OK;
}

/**
 * 从指定的文件当前位置读取这个属性的浮点类型值.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::ReadValueFloat(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	if (fSize == 4) {			// 32 位定点数
		WORD iPart = (WORD)file->ReadInt(sizeof(WORD));
		WORD fPart = (WORD)file->ReadInt(sizeof(WORD));				
		float value = iPart + (((float)fPart) / 0x10000);
		SetValueFloat(value);

	} else if (fSize == 2) {	// 16 位定点数
		BYTE iPart = (BYTE)file->ReadInt(sizeof(BYTE));
		BYTE fPart = (BYTE)file->ReadInt(sizeof(BYTE));				
		float value = iPart + (((float)fPart) / 0x100);
		SetValueFloat(value);

	} else {
		return MP4_ERR_FAILED;
	}

	return MP4_S_OK;
}

/**
 * 从指定的文件当前位置读取这个属性的字符串值.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::ReadValueString(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	UINT size = fSize;
	if (size <= 0) {
		size = fExpectSize;		// 如果没有指定固定长度, 则使用期望长度
		if (size <= 0) {
			return MP4_ERR_INVALID_LENGTH;
		}
	}

	if (size >= MP4_MAX_STRING_LENGTH) {
		return MP4_ERR_PROPERTY_TOO_LARGE;
	}

	char* data = new char[size + 1];
	file->ReadBytes((BYTE*)data, size);
	data[size] = '\0';
	SetValueString(data);
	delete[] data;

	if (fSize == 0) {
		fSize = size;
	}
	return MP4_S_OK;
}

/** 
 * 设置这个属性所期望的大小, 程序将会根据这个值来读取属性的内容, 如字符串属
 * 性可能用它来表示字符串的长度, 表格属性则用来表示表格项的数目, 等等. 
 * @param count 期望的大小.
 */
void Mp4Property::SetExpectSize( UINT count )
{
	fExpectSize = count;
}

/** 设置当前属性的字节数组值. */
void Mp4Property::SetValueBytes( const BYTE* bytes, UINT count )
{
	if (bytes == NULL || count <= 0) {
		return;
	}

	if (fValueBytes && count <= fSize) {
		memcpy(fValueBytes, bytes, count);
	}
}

/** 设置当前属性的浮点型值. */
void Mp4Property::SetValueFloat( float value )
{
	if (fType == PT_INTEGER) {
		fValueInt = (INT64)value;
	} else {
		fValueFloat = value;
	}
}

/** 设置当前属性的整数值. */
void Mp4Property::SetValueInt( INT64 value )
{
	if (fType == PT_FLOAT) {
		fValueFloat = (float)value;
	} else {
		fValueInt = value;
	}
}

/** 设置当前属性的字符串的值. */
void Mp4Property::SetValueString( LPCSTR value )
{
	if (fType != PT_STRING || value == NULL) {
		return;
	}

	UINT size = fSize;
	if (size == 0) {
		// 如果字符串长度不固定, 则根据要设置的值重新分配空间
		if (fValueBytes) {
			delete[] fValueBytes;
			fValueBytes = NULL;
		}

		if (fValueBytes == NULL) {
			size = strlen(value);
			fValueBytes = new char[size + 1];
			memset(fValueBytes, 0, size + 1);
		}
	}
	
	if (fValueBytes) {
		strncpy(fValueBytes, value, size);
	}
}

/**
 * 把这个属性的浮点类型值写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::WriteValueFloat(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	float value = GetValueFloat();
	if (fSize == 4) {
		WORD iPart = (WORD) value;
		WORD fPart = (WORD)((value - iPart) * 0x10000);
		file->WriteInt(iPart, 2);
		file->WriteInt(fPart, 2);

	} else if (fSize == 2) {
		BYTE iPart = (BYTE) value;
		BYTE fPart = (BYTE)((value - iPart) * 0x100);
		file->WriteInt(iPart, 1);
		file->WriteInt(fPart, 1);
	}
	return MP4_S_OK;
}

/**
 * 把这个属性的字符串值写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::WriteValueString(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	UINT size = fSize;
	if (size == 0 && fValueBytes) {
		size = strlen(fValueBytes); // 如果字符串长度不固定, 则取实际的长度
	}
	file->WriteBytes((BYTE*)GetBytes(), size); 
	return MP4_S_OK;
}

/**
 * 把这个属性的内容写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4Property::Write( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	switch (fType) {
	case PT_INTEGER:	file->WriteInt(GetValueInt(), fSize); 			break;
	case PT_BITS:		file->WriteBits((BYTE)GetValueInt(), fSize);	break;
	case PT_BYTES: 		file->WriteBytes((BYTE*)GetBytes(), fSize);		break;
	case PT_FLOAT:		WriteValueFloat(file);							break;
	case PT_STRING: 	WriteValueString(file);							break;
	case PT_TABLE:
	case PT_DESCRIPTOR:		
	case PT_INT_ARRAY:		
	case PT_SIZE_TABLE:		
	case PT_COUNT:		break;
	}

	return MP4_S_OK;
}

/**
 * 计算这个属性要占用的存储空间大小.
 * @return 返回要占用的空间大小. 
 */
UINT Mp4Property::CalculateSize()
{
	if (fSize <= 0) {
		if (fType == PT_STRING && fExpectSize <= 0) {
			return fValueBytes ? strlen(fValueBytes) : 0;
		}
		return fExpectSize; // 当没有指定具体的空间大小时, 则返回期望的空间大小
	
	} else {
		return fSize;
	}
}

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4DescriptorProperty

/**
 * 构建一个新的 Mp4DescriptorProperty 属性.
 * @param name 这个属性的名称.
 */
Mp4DescriptorProperty::Mp4DescriptorProperty(LPCSTR name)
: Mp4Property(PT_DESCRIPTOR, 0, name)
{
	fBuffer		= NULL;
	fBufferSize = 0;
}

Mp4DescriptorProperty::~Mp4DescriptorProperty()
{
	if (fBuffer) {
		delete[] fBuffer;
	}
}

/**
 * 添加一个 Descriptor 属性.
 * @param descriptor 要添加的 Descriptor.
 */
void Mp4DescriptorProperty::AddDescriptor( Mp4Descriptor* descriptor )
{
	if (descriptor) {
		fDescriptors.Add(descriptor);
	}
}

/**
 * 计算这个属性要占用的存储空间大小.
 * @return 返回要占用的空间大小. 
 */
UINT Mp4DescriptorProperty::CalculateSize()
{
	if (fBuffer && fBufferSize > 0) {
		fSize = fBufferSize;

	} else if (fDescriptors.GetCount() > 0) {
		fDescriptors[0]->Build();
		fSize = fDescriptors[0]->GetSize();
	}

	return fSize;
}

/** 返回 Descriptor 项数目. */ 
INT64 Mp4DescriptorProperty::GetIntValue()
{
	return fDescriptors.GetCount();
}

/** 返回 Descriptor 项数目. */ 
UINT Mp4DescriptorProperty::GetDescriptorCount()
{
	return fDescriptors.GetCount();
}

/**
 * 返回指定的索引的 Descriptor 属性
 * @param index 索引值
 * @return 相关的 Descriptor 属性
 */
Mp4DescriptorPtr Mp4DescriptorProperty::GetDescriptor( UINT index )
{
	if (index >= fDescriptors.GetCount()) {
		return NULL;
	}

	return fDescriptors[index];
}

/**
 * 返回指定名称的属性
 * @param name 属性名
 * @return 相关的属性 
 */
Mp4PropertyPtr Mp4DescriptorProperty::GetProperty( LPCSTR name )
{
	if (fDescriptors.GetCount() > 0) {
		return fDescriptors[0]->GetProperty(name);
	}
	return NULL;
}

/**
 * 从指定文件的当前位置读取这个属性的内容.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4DescriptorProperty::Read( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	if (fBuffer) {
		delete[] fBuffer;
		fBuffer = NULL;
	}

	if (fExpectSize > 0) {
		fBuffer = new BYTE[fExpectSize];
		fBufferSize = fExpectSize;
		fSize = fExpectSize;
		file->ReadBytes(fBuffer, fExpectSize);
	}

	return MP4_S_OK;
}

/**
 * 把这个属性的内容写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4DescriptorProperty::Write( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	if (fBuffer && fBufferSize > 0) {
		file->WriteBytes(fBuffer, fBufferSize);
		return MP4_S_OK;
	}

	for (UINT i = 0; i < fDescriptors.GetCount(); i++) {
		Mp4DescriptorPtr descriptor = fDescriptors[i];
		if (descriptor == NULL) {
			break;
		}

		descriptor->Write(file);
	}

	return MP4_S_OK;
}

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4TableProperty

/**
 * 构建一个新的 Mp4TableProperty 属性.
 * @param name 这个属性的名称.
 */
Mp4TableProperty::Mp4TableProperty(LPCSTR name)
: Mp4Property(PT_TABLE, 0, name)
{
	fColumnCount	= 0;
	fRowCount		= 0;
}

/**
 * 添加新的一列.
 * @param name 要添加的列的名称.
 */
void Mp4TableProperty::AddColumn( LPCSTR name )
{
	if (!IsEmpty()) {
		return; // 必须在没有数据前添加列
	}

	fColumnCount++;
	fColumnNames.Add(new Mp4Property(PT_INTEGER, sizeof(UINT), name));
}

/** 添加一行记录. */
void Mp4TableProperty::AddRow( UINT value0, UINT value1, UINT value2, UINT value3)
{
	UINT count = GetColumnCount();
	for (UINT i = 0; i < count; i++) {
		switch (i) {
		case 0:  fTable.Add(value0);	break;
		case 1:  fTable.Add(value1);	break;
		case 2:  fTable.Add(value2);	break;
		case 3:  fTable.Add(value3);	break;
		default: fTable.Add(0);			break;
		}
	}

	fRowCount++;
}

/**
 * 计算这个属性要占用的存储空间大小.
 * @return 返回要占用的空间大小. 
 */
UINT Mp4TableProperty::CalculateSize()
{
	fSize = GetColumnCount() * GetRowCount() * sizeof(UINT);
	return fSize;
}

/** 返回这个表格属性的行数. */
UINT Mp4TableProperty::GetRowCount()
{
	return fRowCount;
}

/** 返回这个表格属性的列数. */
UINT Mp4TableProperty::GetColumnCount()
{
	return fColumnCount;
}

/** 返回这个表格属性的指定列的名称. */
LPCSTR Mp4TableProperty::GetColumnName( UINT colum )
{
	if (colum >= fColumnNames.GetCount()) {
		return "";
	}

	Mp4PropertyPtr property = fColumnNames[colum];
	return (property) ? property->GetBytes() : "";
}

/** 返回这个表格指定的单元格的值. */
UINT Mp4TableProperty::GetValue( UINT row, UINT colum /*= 0*/ )
{
	UINT pos = row * GetColumnCount() + colum;
	if (pos < fTable.GetCount()) {
		return fTable[pos];
	}

	return 0;
}

BOOL Mp4TableProperty::IsEmpty()
{
	return (fRowCount <= 0) ? TRUE : FALSE;
}

UINT Mp4TableProperty::Search( UINT value, UINT colum /*= 0*/ )
{
	int row = 0;
	if (fRowCount <= 0) {
		return row;
	}

	int high = fRowCount - 1;
	int low = 0;
	int mid = 0;
	UINT cell = 0;

	while (low <= high) {
		mid = (low + high) / 2; //获取中间的位置
		cell = GetValue(mid, colum);

		if (cell == value) {
			row = mid; // 找到则返回相应的位置, 
			break;

		} else if (cell > value) {
			high = mid - 1; // 如果比key大，则往低的位置查找  

		} else {
			low = mid + 1;  // 如果比key小，则往高的位置查找  
		}

		row = high;
	}

	return (row < 0) ? 0 : row;
}

/** 设置这个表格指定的单元格的值. */
void Mp4TableProperty::SetValue( UINT index, UINT value, UINT colum /*= 0*/ )
{
	UINT pos = index * GetColumnCount() + colum;
	if (pos < fTable.GetCount()) {
		fTable[pos] = value;
	}
}

/**
 * 从指定文件的当前位置读取这个属性的内容.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4TableProperty::Read( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	fRowCount = fExpectSize;	// 表格行数
	fSize  = CalculateSize();
	if (fSize > MP4_MAX_TABLE_LENGTH) {
		return MP4_ERR_PROPERTY_TOO_LARGE;
	}
	
	UINT count = fSize / sizeof(UINT);

	fTable.Clear();
	fTable.Resize(count);
	if (fTable.GetBuffer() == NULL) {
		return MP4_ERR_OUT_OF_MEMORY;
	}

	BYTE* buffer = new BYTE[fSize];
	UINT ret = file->ReadBytes(buffer, fSize);
	if (ret == fSize) {
		BYTE* p = buffer;
		for (UINT i = 0; i < count; i++) {
			UINT value = Mp4File::ReadInt(p, 4);
			fTable.Add(value);
			p += 4;
		}
	}
	delete[] buffer;
	buffer = NULL;
	return MP4_S_OK;
}

/**
 * 把这个属性的内容写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4TableProperty::Write( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	} 
	
#if 0
	else if (fTable.GetBuffer() == NULL) {
		return MP4_ERR_FAILED;
	}
#endif

	fSize = CalculateSize();
	if (fSize == 0) {
		return MP4_S_OK; // 这是一个空白的表格, 
	}

	UINT count = fSize / sizeof(UINT);
	for (UINT i = 0; i < count; i++) {
		file->WriteInt(fTable[i], sizeof(UINT));
	}

	return MP4_S_OK;
}

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4SizeTableProperty class

/**
 * 构建一个新的 Mp4SizeTableProperty 属性.
 * @param name 这个属性的名称.
 */
Mp4SizeTableProperty::Mp4SizeTableProperty(LPCSTR name)
: Mp4Property(PT_SIZE_TABLE, 0, name)
{
}

Mp4SizeTableProperty::Mp4SizeTableProperty()
: Mp4Property(PT_SIZE_TABLE, 0, "")
{

}

Mp4SizeTableProperty::~Mp4SizeTableProperty()
{
	Clear();
}

/**
 * 添加一个新的实体项.
 * @param bytes 这个 Entry 的内容
 * @param length 这个 Entry 的长度.
 */
void Mp4SizeTableProperty::AddEntry( const BYTE* bytes, WORD length )
{
	if (bytes == NULL || length <= 0) {
		return;
	}

	BYTE* buf = new BYTE[length];
	memcpy(buf, bytes, length);

	fSizeArray.Add(length);
	fBytesArray.Add(buf);
}

/**
 * 计算这个属性要占用的存储空间大小.
 * @return 返回要占用的空间大小. 
 */
UINT Mp4SizeTableProperty::CalculateSize()
{
	UINT size = 0;
	UINT count = GetCount();

	WORD length = 0;
	for (UINT i = 0; i < count; i++) {
		length = 0;
		GetEntry(i, length);
		size += sizeof(WORD);
		size += length;
	}

	fSize = (UINT)size;
	return fSize;
}

void Mp4SizeTableProperty::Clear()
{
	for (UINT i = 0; i < fBytesArray.GetCount(); i++) {
		BYTE* buf = fBytesArray[i];
		if (buf) {
			delete[] buf;
			fBytesArray[i] = NULL;
		}
	}
}

/** 返回这个表格属性中 Entry 元素的个数. */
UINT Mp4SizeTableProperty::GetCount()
{
	return fSizeArray.GetCount();
}

/**
 * 返回指定的索引的实体的内容和长度
 * @param index 索引值
 * @param length 输出参数, 返回这个 Entry 的长度.
 * @return 返回这个 Entry 的内容.
 */
BYTE* Mp4SizeTableProperty::GetEntry( UINT index, WORD& length )
{
	UINT count = fSizeArray.GetCount();
	if (count > fBytesArray.GetCount()) {
		count = fBytesArray.GetCount();
	}

	if (index >= count) {
		return NULL;
	}

	length = fSizeArray[index];
	return fBytesArray[index];
}

/**
 * 从指定文件的当前位置读取这个属性的内容.
 * @param file 要读取的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4SizeTableProperty::Read( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}
	
	UINT count = fExpectSize;	// 表格行数
	for (UINT i = 0; i < count; i++) {
		WORD length = (WORD)file->ReadInt(sizeof(WORD));
		BYTE* bytes = new BYTE[length + 1];
		file->ReadBytes(bytes, length);					
		AddEntry(bytes, length);
		delete[] bytes;
	}

	return MP4_S_OK;
}

/**
 * 把这个属性的内容写入指定的文件的当前位置.
 * @param file 要写入的文件
 * @return 如果成功则返回 MP4_S_OK, 否则返回一个小于 0 的错误码.
 */
int Mp4SizeTableProperty::Write( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_FAILED;
	}

	UINT count = GetCount();
	for (UINT i = 0; i < count; i++) {
		WORD length = 0;
		BYTE *buf = GetEntry(i, length);
		
		file->WriteInt(length, sizeof(WORD));
		file->WriteBytes(buf, length);
	}

	return MP4_S_OK;
}

};

