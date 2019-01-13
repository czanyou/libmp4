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
#include "mp4avc.h"
#include "mp4config.h"

namespace mp4 {

//_______________________________________________________________________________
/////////////////////////////////////////////////////////////////////////////////
// Mp4BitBuffer class

/**
 * 通过指定的数据构建一个新的 BitBuffer
 * @param buffer 数据存放的缓存区
 * @param buflen 缓存区的长度, 单位为字节.
 */
Mp4BitBuffer::Mp4BitBuffer( const BYTE *buffer, UINT buflen )
{
	Init(buffer, buflen);
}

/**
 * 书签操作.
 * @param bSet 如果为 TRUE 表示保存书签, 否则返回恢复书签.
 */
void Mp4BitBuffer::Bookmark( int set )
{
	if (set) {
		fBitsInBufferMark	= fBitsInBuffer;
		fBufferMark			= fBuffer;
		fBufferSizeMark		= fBufferSize;
		fBitsBufferMark		= fBitsBuffer;
		fBookmarkOn			= 1;

	} else {
		fBitsInBuffer		= fBitsInBufferMark;
		fBuffer				= fBufferMark;
		fBufferSize			= fBufferSizeMark;
		fBitsBuffer			= fBitsBufferMark;
		fBookmarkOn			= 0;
	}
}

/**
 * 读取当前位置指定个比特的值.
 * @param bitsCount 要读取的比特的数目, 不能超过 32 位.
 * @return 返回读取的值. 
 */
UINT Mp4BitBuffer::GetBits( UINT bitsCount )
{
	UINT ret = 0;
	static const UINT masks[33] = {
		0x00000000, 0x00000001, 0x00000003, 0x00000007,
		0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
		0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
		0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
		0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
		0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
		0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
		0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
		0xffffffff
	};
	
	if (bitsCount > 32) {
		return 0;
		
	} else if (bitsCount == 0) {
		return 0;
	}
	
	if (fBitsInBuffer >= bitsCount) {  // don't need to read from FILE
		fBitsInBuffer -= bitsCount;
		ret = fBitsBuffer >> fBitsInBuffer;
		// wmay - this gets done below...ret &= msk[numBits];

	} else {
		UINT nbits = bitsCount - fBitsInBuffer;
		if (nbits == 32) {
			ret = 0;

		} else {
			ret = fBitsBuffer << nbits;
		}
		
		switch ((nbits - 1) / 8) {
		case 3:
			nbits -= 8;
			if (fBufferSize < 8) {
				return 0;
			}
			ret |= *fBuffer++ << nbits;
			fBufferSize -= 8;
			// fall through

		case 2:
			nbits -= 8;
			if (fBufferSize < 8) {
				return 0;
			}
			ret |= *fBuffer++ << nbits;
			fBufferSize -= 8;

		case 1:
			nbits -= 8;
			if (fBufferSize < 8) {
				return 0;
			}
			ret |= *fBuffer++ << nbits;
			fBufferSize -= 8;

		case 0:
			break;
		}
		
		if (fBufferSize < nbits) {
			return 0;
		}

		fBitsBuffer		= *fBuffer++;
		fBitsInBuffer	= MIN(8, fBufferSize) - nbits;
		fBufferSize		-= MIN(8, fBufferSize);
		ret |= (fBitsBuffer >> fBitsInBuffer) & masks[nbits];
	}
	return (ret & masks[bitsCount]);
}

/**
 * 返回剩余的比特数.
 */
int Mp4BitBuffer::GetRemainBits( void )
{
	return fBufferSize + fBitsInBuffer;
}

/**
 * 初始化这个 Bitstream
 * @param buffer 数据存放的缓存区
 * @param bit_len 缓存区的长度, 单位为字节.
 */
void Mp4BitBuffer::Init( const BYTE *buffer, UINT bit_len )
{
	fBookmarkOn		= 0;
	fBuffer			= buffer;
	fBufferSize		= bit_len;
	fBitsInBuffer	= 0;
	fBitsBuffer		= 0;
}

/**
 * 返回指定个比特的值, 但不移动指针
 * @param bits 要读取的比特的个数
 * @return UINT 
 */
UINT Mp4BitBuffer::PeekBits( UINT bits )
{
	UINT ret;
	Bookmark(1);
	ret = GetBits(bits);
	Bookmark(0);
	return ret;
}

/**
 * 
 * @param bit 
 * @return void 
 */
void Mp4BitBuffer::SetBit( int bit )
{
	BYTE* buffer = (BYTE*)fBuffer;
	if (fBitsInBuffer > 0) {
		buffer--;
		BYTE mask = ~(0x01 << (fBitsInBuffer - 1));		
		*buffer &= mask;
	} else {
		*buffer &= 0x7f;
	}
}

//_______________________________________________________________________________
/////////////////////////////////////////////////////////////////////////////////
// Mp4Video class

Mp4Video::Mp4Video()
{
	memset(&fSliceHeader, 0, sizeof(fSliceHeader));
	memset(&fSeqParams,   0, sizeof(fSeqParams));
}

Mp4Video::~Mp4Video()
{

}

/** 清除所有的信息. */
void Mp4Video::Clear()
{
	memset(&fSliceHeader, 0, sizeof(fSliceHeader));
	memset(&fSeqParams,   0, sizeof(fSeqParams));
}

/**
 * 返回指定的 NALU 的类型.
 * @param buffer 
 * @return 
 */
BYTE Mp4Video::GetNaluType (const BYTE *sample)
{
	UINT offset = GetStartCodeLength(sample);
	return sample[offset] & 0x1f;
}

/**
 * 返回指定的 slice 类型.
 * @param buffer 
 * @param buflen 
 * @param slice_type 
 * @param noheader 
 * @return 
 */
int Mp4Video::GetSliceType (const BYTE *sample, UINT length, BYTE *slice_type)
{
	if (sample == NULL || length <= 0 || slice_type == NULL) {
		return -1;
	}

	// 跳过 NALU 头部
	UINT offset = GetStartCodeLength(sample) + 1;
	sample += offset;
	length -= offset;

	Mp4BitBuffer buffer;
	buffer.Init(sample, MIN(length, 512) * 8);
	try {
		h264_ue(&buffer);				// first_mb_in_slice
		*slice_type = h264_ue(&buffer); // slice type
		h264_ue(&buffer);				// pic_parameter_set_id	ue(v) 指定使用的图像参数集
		return buffer.GetBits(0 + 4);	// frame_num u(v) 用作一个图像标识符

	} catch (...) {
		return -1;
	}
	return -1;
}

/**
 * 返回指定的 Sample 开始码的长度.
 * @param sample 这个 Sample 的内容
 * @return 返回开始码的长度.
 */
UINT Mp4Video::GetStartCodeLength(const BYTE* sample)
{
	if (sample == NULL) {
		return 0;

	} else if (sample[0] != 0x00 && sample[1] != 0x00) {
		return 0;

	} else if (sample[2] == 0x01) {
		return 3; // [00 00 01]

	} else if (sample[2] == 0x00 && sample[3] == 0x01) {
		return 4; // [00 00 00 01]
	}

	return 0;
}


/** 指数哥伦布编码. 见 H.264 标准 9.1 节. */
UINT Mp4Video::h264_ue (Mp4BitBuffer *buffer)
{
	if (buffer == NULL) {
		return 0;
	}

	static BYTE mp4ExpGolombBits[256] = {
		8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 
		3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 
		2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 
		1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
		0, 
	};

	UINT read = 0;
	UINT bits = 0;
	bool done = false;

	// we want to read 8 bits at a time - if we don't have 8 bits, 
	// read what's left, and shift.  The exp_golomb_bits calc remains the
	// same.
	while (done == false) {
		int bits_left = buffer->GetRemainBits();
		if (bits_left < 8) {
			read = buffer->PeekBits(bits_left) << (8 - bits_left);
			done = true;

		} else {
			read = buffer->PeekBits(8);
			if (read == 0) {
				buffer->GetBits(8);
				bits += 8;
			} else {
				done = true;
			}
		}
	}

	BYTE coded = mp4ExpGolombBits[read];
	buffer->GetBits(coded);
	bits += coded;

	//  printf("ue - bits %d\n", bits);
	return buffer->GetBits(bits + 1) - 1;
}

/** 有符号指数哥伦布编码. 见 H.264 标准 9.1.1 节.  */
int Mp4Video::h264_se (Mp4BitBuffer *buffer) 
{
	if (buffer == NULL) {
		return 0;
	}

	UINT ret = h264_ue(buffer);
	if ((ret & 0x1) == 0) {
		ret >>= 1;
		int temp = 0 - ret;
		return temp;
	}
	return (ret + 1) >> 1;
}

/**
 * 解析指定的 NALU 单元的头信息. 
 *
 * @param sample 
 * @param length 
 * @return int 
 */
int Mp4Video::ParseHeader( const BYTE* sample, int length )
{
	if (sample == NULL || length <= 0) {
		return -1;
	}

	UINT offset = GetStartCodeLength(sample);
	sample += offset;
	length -= offset;

	int ret  = 0;
	BYTE type	 = GetNaluType(sample);
	BOOL isSlice = FALSE;
	switch (type) {
	case H264_NAL_TYPE_ACCESS_UNIT:
	case H264_NAL_TYPE_END_OF_SEQ:
	case H264_NAL_TYPE_END_OF_STREAM:
		ret = 1;
		break;

	case H264_NAL_TYPE_SEQ_PARAM:
		if (ReadSeqParams(sample, length, &fSeqParams) < 0) {
			return -1;
		}
		break;

	case H264_NAL_TYPE_NON_IDR_SLICE:
	case H264_NAL_TYPE_DP_A_SLICE:
	case H264_NAL_TYPE_DP_B_SLICE:
	case H264_NAL_TYPE_DP_C_SLICE:
	case H264_NAL_TYPE_IDR_SLICE:
		isSlice = TRUE;
		break;
	}

	fSliceHeader.nal_unit_type = type;
	fSliceHeader.is_slice = isSlice;

	if (isSlice) {
		return ReadSliceInfo(sample, length, &fSeqParams, &fSliceHeader);
	}

	return ret;
}

/** 解析序列信息. */
int Mp4Video::ReadSeqParams (const BYTE *sample, UINT length, H264SeqParams *sps)
{
	if (sample == NULL || length <= 0 || sps == NULL) {
		return -1;
	}

	// 跳过 NALU 头部
	UINT offset = GetStartCodeLength(sample) + 1;
	sample += offset;
	length -= offset;

	Mp4BitBuffer buffer;	
	buffer.Init(sample, MIN(length, 512) * 8);
	try {
		sps->profile = buffer.GetBits(8);	// 是指比特流所遵守的配置和级别
		buffer.GetBits(1 + 1 + 1 + 1 + 4);	// 
		sps->level = buffer.GetBits(8);		// 是指比特流所遵守的配置和级别

		h264_ue(&buffer); // seq_parameter_set_id	// 用于识别图像参数集所指的序列参数集
		if (sps->profile == 100 || sps->profile == 110 ||
			sps->profile == 122 || sps->profile == 144) {
				sps->chroma_format_idc = h264_ue(&buffer);	// 与亮度取样对应的色度取样
				if (sps->chroma_format_idc == 3) {
					sps->residual_colour_transform_flag = buffer.GetBits(1);
				}

				sps->bit_depth_luma_minus8	 = h264_ue(&buffer); // 是指亮度队列样值的比特深度以及亮度量化参数范围的取值偏移
				sps->bit_depth_chroma_minus8 = h264_ue(&buffer); // 是指色度队列样值的比特深度以及色度量化参数范围的取值偏移
				sps->qpprime_y_zero_transform_bypass_flag = buffer.GetBits(1);
				sps->seq_scaling_matrix_present_flag = buffer.GetBits(1);
				if (sps->seq_scaling_matrix_present_flag) {
					for (UINT ix = 0; ix < 8; ix++) {
						if (buffer.GetBits(1)) {
							ScalingList(ix < 6 ? 16 : 64, &buffer);
						}
					}
				}
		}

		sps->log2_max_frame_num_minus4	= h264_ue(&buffer);
		sps->pic_order_cnt_type			= h264_ue(&buffer);	// 是指解码图像顺序的计数方法
		if (sps->pic_order_cnt_type == 0) {
			sps->log2_max_pic_order_cnt_lsb_minus4 = h264_ue(&buffer);

		} else if (sps->pic_order_cnt_type == 1) {
			sps->delta_pic_order_always_zero_flag = buffer.GetBits(1);
			sps->offset_for_non_ref_pic			= h264_se(&buffer); // offset_for_non_ref_pic
			sps->offset_for_top_to_bottom_field = h264_se(&buffer); // offset_for_top_to_bottom_field
			sps->pic_order_cnt_cycle_length		= h264_ue(&buffer); // poc_cycle_length
			for (UINT ix = 0; ix < sps->pic_order_cnt_cycle_length; ix++) {
				sps->offset_for_ref_frame[MIN(ix,255)] = h264_se(&buffer); // offset for ref fram -
			}
		}

		h264_ue(&buffer);	// num_ref_frames
		buffer.GetBits(1);	// gaps_in_frame_num_value_allowed_flag
		UINT PicWidthInMbs = h264_ue(&buffer) + 1;		 // 加 1 是指以宏块为单元的每个解码图像的宽度。
		sps->pic_width = PicWidthInMbs * 16;
		UINT PicHeightInMapUnits = h264_ue(&buffer) + 1; // 加 1 表示以条带组映射为单位的一个解码帧或场的高度。

		sps->frame_mbs_only_flag = buffer.GetBits(1); // 等于 0 表示编码视频序列的编码图像可能是编码场或编码帧
		sps->pic_height = (2 - sps->frame_mbs_only_flag) * PicHeightInMapUnits * 16;

	} catch (...) {
		return -1;
	}
	return 0;
}

/** 解析 slice 的头信息. */
int Mp4Video::ReadSliceInfo (const BYTE *sample, UINT length, H264SeqParams *seq, H264SliceHeader *dec)
{
	if (sample == NULL || length <= 0 || seq == NULL || dec == NULL) {
		return -1;
	}

	// 跳过 NALU 头部
	UINT offset = GetStartCodeLength(sample) + 1;
	sample += offset;
	length -= offset;

	Mp4BitBuffer buffer;
	buffer.Init(sample, MIN(length, 512) * 8);
	try {
		dec->field_pic_flag			= 0;
		dec->bottom_field_flag		= 0;
		dec->delta_pic_order_cnt[0] = 0;
		dec->delta_pic_order_cnt[1] = 0;

		h264_ue(&buffer);	// first_mb_in_slice 	ue(v) 表示在条带中第一个宏块的地址
		dec->slice_type = h264_ue(&buffer); //  	ue(v) 表示条带的编码类型
		h264_ue(&buffer);	// pic_parameter_set_id	ue(v) 指定使用的图像参数集
		dec->frame_num = buffer.GetBits(seq->log2_max_frame_num_minus4 + 4); // u(v) 用作一个图像标识符

		if (!seq->frame_mbs_only_flag) {
			dec->field_pic_flag = buffer.GetBits(1);		// u(1) 等于 1 表示该条带是一个编码场的条带
			if (dec->field_pic_flag) {
				dec->bottom_field_flag = buffer.GetBits(1);	// u(1) 等于 1 表示该条带是一个编码底场的一部分
			}
		}

		if (dec->nal_unit_type == H264_NAL_TYPE_IDR_SLICE) {
			dec->idr_pic_id = h264_ue(&buffer);		// ue(v) 标识一个 IDR 图像。
		}

		switch (seq->pic_order_cnt_type) {
		case 0:
			// pic_order_cnt_lsb	u(v) 表示一个编码帧的顶场或一个编码场的图像顺序数对 MaxPicOrderCntLsb  取模
			dec->pic_order_cnt_lsb = buffer.GetBits(seq->log2_max_pic_order_cnt_lsb_minus4 + 4);
			if (seq->pic_order_present_flag && !dec->field_pic_flag) {
				dec->delta_pic_order_cnt_bottom = h264_se(&buffer);	// se(v) 表示一个编码帧的底场和顶场的图像顺序数之间的差
			}
			break;
		case 1:
			if (!seq->delta_pic_order_always_zero_flag) {
				dec->delta_pic_order_cnt[0] = h264_se(&buffer);
			}

			if (seq->pic_order_present_flag && !dec->field_pic_flag) {
				dec->delta_pic_order_cnt[1] = h264_se(&buffer);
			}
			break;
		}

	} catch (...) {
		return -1;
	}
	return 0;
}

/**
 * 
 * @param sizeOfScalingList 
 * @param bs 
 * @return void 
 */
void Mp4Video::ScalingList (UINT sizeOfScalingList, Mp4BitBuffer *buffer)
{
	UINT lastScale = 8, nextScale = 8;
	UINT j;

	for (j = 0; j < sizeOfScalingList; j++) {
		if (nextScale != 0) {
			int deltaScale = h264_se(buffer);
			nextScale = (lastScale + deltaScale + 256) % 256;
		}

		if (nextScale == 0) {
			// lastScale = lastScale;

		} else {
			lastScale = nextScale;
		}
	}
}

};
