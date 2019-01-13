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
#include "mp4atom.h"
#include "mp4factory.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Atom class

Mp4Atom::Mp4Atom()
{
	Reset();
}

/** 
 * 构建 Mp4Atom
 * @param type 这个 atom 的类型, 一般 atom 类型须为 4 个 ASCII 字符
 *	组成, 如 "moov".
 */
Mp4Atom::Mp4Atom( LPCSTR type )
{
	Reset();
	if (type) {
		memcpy(fType, type, 4);
		Mp4Factory factory;
		factory.AddProperties(this, 0);
	}
}

Mp4Atom::~Mp4Atom()
{
	fParentAtom	= NULL;
}

/**
 * 添加一个指定的名称的子 ATOM 节点. 
 * @param path 要添加的子 ATOM 节点的路径和类型, 如 'trak.tkhd'
 * @param index 插入的位置, -1 表示添加到其他节点的后面.
 * @return 返回添加的节点的指针, 如果没有添加成功则返回一个空指针.
 */
Mp4AtomPtr Mp4Atom::AddChildAtom( LPCSTR path, UINT index )
{
	if (path == NULL || !IsValidName(path)) {
		return NULL; // 无效的 Atom 名称
	}

	// 多级 Atom 路径
	if (strlen(path) > 4 && path[4] == '.') {
		Mp4AtomPtr child = GetChildAtom(path);
		if (child) {
			return child->AddChildAtom(path + 5, index);
		}
		return NULL;

	} else {
		Mp4AtomPtr atom = new Mp4Atom(path);
		atom->SetParent(this);
		atom->Init(0);

		fChildAtoms.Insert(atom, index);
		return atom;
	}
}

/** 添加一个指定的子 Atom. */
void Mp4Atom::AddChildAtom( Mp4Atom* atom )
{
	if (atom) {
		fChildAtoms.Add(atom);
	}
}

/** 
 * 添加指定类型和名称的属性. 
 * @param type 要添加的属性的类型
 * @param size 要添加的属性的大小, 单位为字节
 * @param name 要添加的属性的名称
 * @return 返回添加的属性的指针, 如果没有添加成功则返回一个空指针.
 */
Mp4PropertyPtr Mp4Atom::AddProperty(Mp4PropertyType type, int size, LPCSTR name)
{
	Mp4PropertyPtr property;
	if (type == PT_SIZE_TABLE) {
		property = new Mp4SizeTableProperty(name);

	} else if (type == PT_DESCRIPTOR) {
		property = new Mp4DescriptorProperty(name);

	} else if (type == PT_TABLE) {
		return NULL; // 不支持通过这个方法添加这个类型的属性

	} else if (type == PT_INT_ARRAY) {
		return NULL; // 不支持通过这个方法添加这个类型的属性

	} else {
		property = new Mp4Property(type, size, name);
	}

	fProperties.Add(property);
	return property;
}

/** 添加 version 和 flags 两个公共的属性. */
void Mp4Atom::AddVersionAndFlags() 
{
	AddProperty(PT_INTEGER, 1, "version");
	AddProperty(PT_INTEGER, 3, "flags");
}

/** 计算这个节点当前需要占用的存储空间的大小. */
INT64 Mp4Atom::CalculateSize()
{
	INT64 size = MP4_ATOM_HEADER_SIZE; // Atom 头占用空间
	UINT  i = 0;

	// 所有属性占用空间
	UINT bits = 0;
	UINT count = fProperties.GetCount();
	for (i = 0; i < count; i++) {
		Mp4PropertyPtr property = fProperties[i];
		if (property == NULL) {
			break;
		}

		if (property->GetType() == PT_BITS) {
			bits += property->GetSize();

		} else {
			size += property->CalculateSize();
		}
	}

	if (bits > 0) {
		size += (bits / 8);
	}

	// 所有子 Atom 占用空间
	count = fChildAtoms.GetCount();
	for (i = 0; i < count; i++) {
		Mp4AtomPtr atom = fChildAtoms[i];
		if (atom) {
			size += atom->CalculateSize();
		}
	}

	fSize = size;
	return fSize;
}

/** 清除所有的资源. */
void Mp4Atom::Clear()
{
	ClearChildAtoms();
	fProperties.Clear();

	fExpectChild = FALSE;
	fParentAtom = NULL;	
	fStart		= 0;
	fSize		= 0;
}

/** 清除所有的子节点. */
void Mp4Atom::ClearChildAtoms()
{
	UINT count = fChildAtoms.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4AtomPtr atom = fChildAtoms[i];
		if (atom) {
			atom->Clear();
		}
	}

	fChildAtoms.Clear();	
}

/** 
 * 查找指定的名称的子 atom 节点. 
 * @param path 要查找的节点的名称, 多级节点名称以 "." 隔开, 如 "moov.trak".
 * @return 返回找到的节点的指针, 如果没有找到则返回一个空指针.
 */
Mp4AtomPtr Mp4Atom::FindAtom( LPCSTR path )
{
	if (path == NULL || *path == '\0') {
		return NULL;
	}

	LPCSTR p = strstr(path, ".");
	if (p) {
		Mp4AtomPtr child = GetChildAtom(path);
		return child ? child->FindAtom(p + 1) : NULL;

	} else {
		return GetChildAtom(path);
	}
}

/** 查找并返回指定的名称的属性. */
Mp4PropertyPtr Mp4Atom::FindProperty( LPCSTR path )
{
	if (path == NULL || *path == '\0') {
		return NULL;
	}

	LPCSTR p = strstr(path, ".");
	if (p) {
		// 查找指定名称的子节点
		Mp4AtomPtr child = GetChildAtom(path);
		return child ? child->FindProperty(p + 1) : NULL;
	}

	// 返回指定名称的属性
	UINT count = fProperties.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4PropertyPtr property = fProperties[i];
		if (property == NULL) {
			break;

		} else if (property->GetType() == PT_DESCRIPTOR) {
			Mp4DescriptorProperty* descriptor = NULL;
			descriptor = (Mp4DescriptorProperty*)property.ptr;
			return descriptor->GetProperty(path);

		} else {
			LPCSTR name = property->GetName();
			if (name && !strcasecmp(path, name)) {
				return property;
			}
		}
	}

	return NULL;	
}

/** 
 * 返回指定的名称的子节点, 注意这个方法只会返回这一级节点的子节点.
 * @param name 要查找的节点的名称. 
 * @return 返回找到的节点的指针, 如果没有找到则返回一个空指针.
 */
Mp4AtomPtr Mp4Atom::GetChildAtom( LPCSTR name )
{
	if (name == NULL || !IsValidName(name)) {
		return NULL;
	}

	UINT type = Mp4Utils::AtomId(name);
	UINT count = fChildAtoms.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4AtomPtr atom = fChildAtoms[i];
		if (atom && Mp4Utils::AtomId(atom->fType) == type) {
			return atom;
		}
	}

	return NULL;
}

/** 返回指定的索引的子节点. */
Mp4AtomPtr Mp4Atom::GetChildAtom( UINT index )
{
	if (index >= fChildAtoms.GetCount()) {
		return NULL;
	}

	return fChildAtoms[index];
}

/** 返回指定的名称的 atom 的索引位置. */
int Mp4Atom::GetChildAtomIndex( LPCSTR name )
{
	if (name == NULL || *name == '\0') {
		return -1;
	}

	UINT type = Mp4Utils::AtomId(name);
	UINT count = fChildAtoms.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4AtomPtr atom = fChildAtoms[i];
		if (atom && Mp4Utils::AtomId(atom->fType) == type) {
			return i;
		}
	}

	return -1;
}

/** 返回这个 Atom 的子 Atom 的个数. */
UINT Mp4Atom::GetChildrenCount()
{
	return fChildAtoms.GetCount();
}

INT64 Mp4Atom::GetEnd() const
{
	return fStart + fSize;
}

///< 返回这个 Atom 的父 Atom
Mp4AtomPtr Mp4Atom::GetParent()
{
	return fParentAtom;
}

/** 返回指定的索引的属性. */
Mp4PropertyPtr Mp4Atom::GetProperty( UINT index )
{
	if (index >= fProperties.GetCount()) {
		return NULL;
	}

	return fProperties[index];
}

BYTE* Mp4Atom::GetPropertyBytes( LPCSTR name )
{
	Mp4PropertyPtr property = FindProperty(name);
	return property ? (BYTE*)property->GetBytes() : 0;
}

/** 返回这个 Atom 的属性的个数. */
UINT Mp4Atom::GetPropertyCount()
{
	return fProperties.GetCount();
}

/** 返回指定的属性的值. 如果不存在则返回 0.0. */
float Mp4Atom::GetPropertyFloat( LPCSTR name )
{
	Mp4PropertyPtr property = FindProperty(name);
	if (property) {
		return property->GetValueFloat();
	}
	return 0.0;
}

/** 返回指定的属性的值. 如果不存在则返回 0. */
UINT Mp4Atom::GetPropertyInt( LPCSTR name )
{
	Mp4PropertyPtr property = FindProperty(name);
	return property ? (UINT)property->GetValueInt() : 0;
}

/** 返回指定的属性的值. 如果不存在则返回 0. */
INT64 Mp4Atom::GetPropertyInt64( LPCSTR name )
{
	Mp4PropertyPtr property = FindProperty(name);
	return property ? property->GetValueInt() : 0;
}

/** 返回指定的属性的值. */
int Mp4Atom::GetPropertyString( LPCSTR name, char* buf, UINT bufLen )
{
	Mp4PropertyPtr property = FindProperty(name);
	if (property == NULL || buf == NULL || bufLen <= 1) {
		return 0;
	}

	return property->GetValueString(buf, bufLen);
}

UINT Mp4Atom::GetRemaining( Mp4File* file )
{
	if (file == NULL) {
		return 0;
	}

	INT64 position = file->GetPosition();
	if (position < fStart) {
		return 0;
	}

	INT64 end = fStart + fSize;
	if (position >= end) {
		return 0;
	}

	return UINT(end - position);
}

///< 返回这个 Atom 的大小, 单位为字节.
INT64 Mp4Atom::GetSize() const
{
	return fSize;
}

///< 返回这个 Atom 在文件中的开始位置.
INT64 Mp4Atom::GetStart() const
{
	return fStart;
}

///< 返回这个 Atom 的类型
LPCSTR Mp4Atom::GetType() const
{
	return fType;
}

/** 初始化这个 atom, 添加必须的子 atom 节点, 以及设置默认的属性值等. */
int Mp4Atom::Init( BYTE version )
{
	Mp4Factory factory;
	return factory.InitProperties(this, version);
}

BOOL Mp4Atom::IsValidName(LPCSTR path)
{
	if (path == NULL || *path == '\0') {
		return FALSE;

	} else if (strlen(path) <= 4) {
		return TRUE;

	} else if ((path[4] != '\0' && path[4] != '.')) {
		return FALSE;
	}

	return TRUE;
}

/** 读取 ATOM 的内容. */
int Mp4Atom::Read( Mp4File* file, UINT flags )
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	int ret = MP4_S_OK;
	if (fProperties.GetCount() > 0) {
		ret = ReadProperties(file);
	}

	if ((ret == MP4_S_OK) && fExpectChild) {
		ret = ReadChildAtoms(file, flags);
	}

	return ret;
}

/** 读取所有的子节点. */
int Mp4Atom::ReadChildAtoms(Mp4File* file, UINT flags)
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	INT64 leftover = GetRemaining(file); // 剩余的数据的长度
	if (leftover < 8) {
		return MP4_ERR_READ;
	}
	int ret = MP4_S_OK;

	// 读取所有的子节点
	while (leftover > 8) {
		INT64 start = file->GetPosition();	

		// ATOM 长度
		INT64 size  = file->ReadInt(4);
		if (size == 0) {
			size = file->GetFileSize() - start;
		}

		if (size < 8) {
			ret = MP4_ERR_READ;
			break;
		}

		// ATOM 类型
		char type[5];
		UINT ret = file->ReadBytes((BYTE*)type, 4);
		if (ret != 4) {
			ret = MP4_ERR_READ;
			break;
		}
		type[4] = '\0';

		Mp4AtomPtr atom = new Mp4Atom(type);
		atom->SetParent(this);
		atom->SetStart(start);
		atom->SetSize(size);
		fChildAtoms.Add(atom);

		// 读取这个 ATOM 节点的属性和子节点
		if (flags == 0) {
			ret = atom->Read(file);
			if (ret != MP4_S_OK) {
				break;
			}
		}

		// 跳过没有读取完的数据
		atom->ReadSkip(file);
		leftover -= size;
	}

	return ret;
}

/** 读取这个节点的所有属性的值. */
int Mp4Atom::ReadProperties(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	int ret = MP4_S_OK;
	UINT count = fProperties.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4PropertyPtr property = fProperties[i];
		if (property == NULL) {
			ret = MP4_ERR_NULL_PROPERTY;
			break;
		}

		// 读取下一个属性期望的大小
		Mp4PropertyType type = property->GetType();
		if (type == PT_TABLE || type == PT_SIZE_TABLE) {
			int count = 0;
			if (i > 0) {
				// Table 属性前有一个表示长度/个数的属性
				Mp4PropertyPtr countProperty = fProperties[i - 1];
				if (countProperty) {
					count = (int)countProperty->GetValueInt();
				}
			}

			if (count > 0) {
				property->SetExpectSize(count);

			} else {
				// TODO: 
			}

		} else if (type == PT_STRING) {
			if (property->GetSize() == 0) {
				property->SetExpectSize(GetRemaining(file));
			}

		} else if (type == PT_DESCRIPTOR) {
			property->SetExpectSize(GetRemaining(file));

		} else if (property->GetSize() == 0) {
			property->SetExpectSize(GetRemaining(file));
		}

		// Read
		ret = property->Read(file);
		if (ret != MP4_S_OK) {
			break;
		}
	}

	return ret;
}

/** 跳过没有读取完的数据. */
int Mp4Atom::ReadSkip( Mp4File* file )
{
	if (file == NULL) {
		return 0;
	}

	INT64 end = fStart + fSize;
	INT64 posotion = file->GetPosition();
	if ((end > 0) && (end != posotion)) {
		file->SetPosition(end);
		return int(end - posotion);
	}

	return 0;
}

void Mp4Atom::RemoveChildAtom( LPCSTR name )
{
	UINT index = GetChildAtomIndex(name);
	SetChildAtom(index, NULL);
}

/** 重置所有成员变量. */
void Mp4Atom::Reset()
{
	fSize	= 0;
	fStart	= 0;
	fExpectChild = FALSE;
	memset(fType, 0, sizeof(fType));
}

/** 用指定的 atom 替换指定位置的 atom. */
void Mp4Atom::SetChildAtom( UINT index, Mp4Atom* atom )
{
	if (atom && index < fChildAtoms.GetCount()) {
		fChildAtoms[index] = atom;
	}
}

/** 设置这个 Atom 的父 Atom */
void Mp4Atom::SetParent( Mp4Atom* parent )
{
	fParentAtom = parent;
}

/** 设置指定的属性的值. */
void Mp4Atom::SetPropertyFloat( LPCSTR name, float value )
{
	Mp4PropertyPtr property = FindProperty(name);
	if (property) {
		property->SetValueFloat(value);
	}
}

/** 设置指定的属性的值. */
void Mp4Atom::SetPropertyInt( LPCSTR name, INT64 value )
{
	Mp4PropertyPtr property = FindProperty(name);
	if (property) {
		property->SetValueInt(value);
	}
}

/** 设置指定的属性的值. */
void Mp4Atom::SetPropertyString( LPCSTR name, LPCSTR value )
{
	Mp4PropertyPtr property = FindProperty(name);
	if (property) {
		property->SetValueString(value);
	}	
}

/** 设置这个节点的长度. */
void Mp4Atom::SetSize( INT64 size )
{
	fSize = size; 
}

/** 设置这个节点的开始位置. */
void Mp4Atom::SetStart( INT64 start )
{
	fStart = start; 
}

/** 
 * 把这个 atom 的内容写入指定的文件当前位置. 
 * @param file 要写入的 MP4 文件
 * @return 如果成功则返回 MP4_S_OK(0), 否则返回一个代表错误码的负数.
 */
int Mp4Atom::Write( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	UINT type = Mp4Utils::AtomId(fType);
	if (type == Mp4Utils::AtomId("root") || type == Mp4Utils::AtomId("mdat")) {
		return MP4_ERR_FAILED; // 不可以直接写这两个 atom 
	}

	int ret = WriteHeader(file); // 写这个 ATOM 的头部

	if (type == Mp4Utils::AtomId("free")) {
		return WriteFreeAtom(file);
	}

	if (ret == MP4_S_OK) {
		ret = WriteProperties(file); // 写这个节点的属性
	}

	if (ret == MP4_S_OK) {
		ret = WriteChildAtoms(file); // 写这个节点的子节点
	}

	if (ret == MP4_S_OK) {
		ret = WriteSize(file); // 写这个节点实际的长度等, 完成整个节点的写操作.
	}

	return ret;
}

/** 写这个节点的所有子节点到文件中. */
int Mp4Atom::WriteChildAtoms(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	int ret = MP4_S_OK;	
	UINT count = fChildAtoms.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4AtomPtr atom = fChildAtoms[i];
		if (atom == NULL) {
			continue;
		}

		ret = atom->Write(file);
		if (ret != MP4_S_OK) {
			break;
		}
	}
	return ret;
}

int Mp4Atom::WriteFreeAtom(Mp4File* file)
{
	int size = (int)GetPropertyInt("size");
	if (size < 4 || size > 1024 * 1024) {
		size = 4;
	}	

	BYTE buf[1024];
	memset(buf, 0, sizeof(buf));
	int leftover = size;
	while (leftover > 0) {
		int write_size = (leftover > 1024) ? 1024 : leftover;
		file->WriteBytes(buf, write_size);
		leftover -= write_size;
	}
	return WriteSize(file);
}

/** 开始写入文件, 写 atom 公共的头部. */
int Mp4Atom::WriteHeader( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_WRITE;
	}

	fStart	 = file->GetPosition(); // 记录这个 atom 在文件中的开始位置

	UINT ret = file->WriteInt(fSize, 4);
	ret		+= file->WriteBytes((BYTE*)fType, 4);
	return (ret == 8) ? MP4_S_OK : MP4_ERR_WRITE;
}

/** 写这个节点的所有属性到文件中. */
int Mp4Atom::WriteProperties(Mp4File* file)
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	int ret = MP4_S_OK;
	UINT count = fProperties.GetCount();
	for (UINT i = 0; i < count; i++) {
		Mp4PropertyPtr property = fProperties[i];
		if (property == NULL) {
			ret = MP4_ERR_NULL_PROPERTY;
			break;
		}

		if ((ret = property->Write(file)) != MP4_S_OK) {
			break;
		}
	}

	return ret;
}

/** 完成写入, 主要是写入这个 atom 的实际长度. */
int Mp4Atom::WriteSize( Mp4File* file )
{
	if (file == NULL) {
		return MP4_ERR_NULL_FILE;
	}

	INT64 position = file->GetPosition();	// 记录这个 atom 在文件中的结束位置
	fSize = position - fStart;

	// 重新写这个节点的长度
	file->SetPosition(fStart);
	UINT ret = file->WriteInt(fSize, 4);

	// 移回节点的结束位置
	file->SetPosition(position);

	return (ret == 4) ? MP4_S_OK : MP4_ERR_WRITE;
}

};

