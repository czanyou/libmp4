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

#include "mp4factory.h"

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Factory class

Mp4Factory::Mp4Factory(void)
{
}

Mp4Factory::~Mp4Factory(void)
{
}

/** 
 * 初始化这个 atom 的属性列表等属性. 
 * @param version 这个 atom 的版本号
 * @return 如果成功则返回 MP4_S_OK(0), 否则返回一个小于 0 的错误.
 */
int Mp4Factory::AddProperties(Mp4Atom* atom, BYTE version)
{
	if (atom == NULL) {
		return 0;
	}

	LPCSTR name = atom->GetType();
	if (*name == '\0') {
		return MP4_ERR_FAILED; // 没有设置 atom 的类型
	}

	const UINT type = Mp4Utils::AtomId(name);

	if (*name == 'a') {
		// AVC atom
		if (type == Mp4Utils::AtomId("avc1")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		4, "temporalQuality");
			atom->AddProperty(PT_INTEGER,		4, "spatialQuality");
			atom->AddProperty(PT_INTEGER,		2, "width");
			atom->AddProperty(PT_INTEGER,		2, "height");
			atom->AddProperty(PT_FLOAT,			4, "hor");
			atom->AddProperty(PT_FLOAT,			4, "ver");
			atom->AddProperty(PT_INTEGER,		4, "dataSize");
			atom->AddProperty(PT_INTEGER,		2, "frameCount");
			atom->AddProperty(PT_STRING,	   32, "compressorName");
			atom->AddProperty(PT_INTEGER,		2, "depth");
			atom->AddProperty(PT_INTEGER,		2, "colorTable");
			atom->fExpectChild = TRUE;

		// AVC atom
		} else if (type == Mp4Utils::AtomId("avcC")) {
			atom->AddProperty(PT_INTEGER,		1, "configurationVersion");
			atom->AddProperty(PT_INTEGER,		1, "AVCProfileIndication");
			atom->AddProperty(PT_INTEGER,		1, "profile_compatibility");
			atom->AddProperty(PT_INTEGER,		1, "AVCLevelIndication");
			atom->AddProperty(PT_INTEGER,		1, "lengthSizeMinusOne");
			
			atom->AddProperty(PT_BITS,			3, "reserved2");
			atom->AddProperty(PT_BITS,			5, "numOfSequenceParameterSets");
			atom->AddProperty(PT_SIZE_TABLE,	0, "sequenceEntries");
			atom->AddProperty(PT_INTEGER,		1, "numOfPictureParameterSets");
			atom->AddProperty(PT_SIZE_TABLE,	0, "pictureEntries");

		} else if (type == Mp4Utils::AtomId("alaw")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");
		}

	} else if (*name == 'b') {
		// Data information atom
		if (type == Mp4Utils::AtomId("btrt")) {
			atom->AddProperty(PT_INTEGER,		4, "bufferSizeDB");
			atom->AddProperty(PT_INTEGER,		4, "maxBitrate");
			atom->AddProperty(PT_INTEGER,		4, "avgBitrate");
		}

	} else if (*name == 'd') {

		// Data information atom
		if (type == Mp4Utils::AtomId("dinf")) {
			atom->fExpectChild = TRUE;

		// AMR audio atom
		} else if (type == Mp4Utils::AtomId("damr")) {
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		1, "decoderVersion");
			atom->AddProperty(PT_INTEGER,		2, "modeSet");
			atom->AddProperty(PT_INTEGER,		1, "modeChangePeriod");
			atom->AddProperty(PT_INTEGER,		1, "framesPerSample");

		// Data reference atom
		} else if (type == Mp4Utils::AtomId("dref")) {
			atom->AddVersionAndFlags();			
			atom->AddProperty(PT_INTEGER,		4, "entryCount");
			atom->fExpectChild = TRUE;
		}

	} else if (*name == 'e') {
		// MPEG-4 elementary stream descriptor atom
		if (type == Mp4Utils::AtomId("esds")) {
			atom->AddVersionAndFlags();
			
			Mp4DescriptorProperty* property = new Mp4DescriptorProperty("descriptor");
			property->AddDescriptor(new Mp4Descriptor(Mp4ESDescrTag));
			atom->fProperties.Add(property);
		}

	} else if (*name == 'f') {
		// File type atom
		if (type == Mp4Utils::AtomId("ftyp")) {
			atom->AddProperty(PT_STRING,		4, "majorBrand");
			atom->AddProperty(PT_INTEGER,		4, "minorVersion");
			atom->AddProperty(PT_STRING,		0, "brands");

		// Free atom
		} else if (type == Mp4Utils::AtomId("free")) {
			atom->AddProperty(PT_INTEGER,		4, "size");
		}

	} else if (*name == 'g') {

		if (type == Mp4Utils::AtomId("g726")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");

		} else if (type == Mp4Utils::AtomId("gps ")) {
			atom->AddProperty(PT_STRING,		0, "track");
		}

	} else if (*name == 'h') {
		// Handler reference atom
		if (type == Mp4Utils::AtomId("hdlr")) {
			atom->AddVersionAndFlags();
			atom->AddProperty(PT_STRING,		4, "type");
			atom->AddProperty(PT_STRING,		4, "handlerType");
			atom->AddProperty(PT_STRING,		4, "manufacturer");
			atom->AddProperty(PT_STRING,		4, "reserved1");
			atom->AddProperty(PT_STRING,		4, "reserved2");
			atom->AddProperty(PT_STRING,	   21, "name");
		}

	} else if (*name == 'i') {
		// Descriptor atom
		if (type == Mp4Utils::AtomId("iods")) {
			atom->AddVersionAndFlags();

			Mp4DescriptorProperty* property = new Mp4DescriptorProperty("descriptor");
			property->AddDescriptor(new Mp4Descriptor(Mp4FileIODescrTag));
			atom->fProperties.Add(property);

		} else if (type == Mp4Utils::AtomId("indx")) {
			Mp4TableProperty *table = new Mp4TableProperty("entries");
			table->AddColumn("id");
			table->AddColumn("flags");
			table->AddColumn("offset");
			table->AddColumn("duration");
			atom->fProperties.Add(table);
		}

	} else if (*name == 'm') {
		// Movie atom
		if (type == Mp4Utils::AtomId("moov")) {
			atom->fExpectChild = TRUE;

		// Media atom
		} else if (type == Mp4Utils::AtomId("mdia")) {
			atom->fExpectChild = TRUE;			

		} else if (type == Mp4Utils::AtomId("meta")) {
			atom->AddProperty(PT_BYTES,			0, "data");

		// Movie header atom
		} else if (type == Mp4Utils::AtomId("mvhd")) {
			atom->AddVersionAndFlags();
	
			atom->AddProperty(PT_INTEGER,		4, "creationTime");
			atom->AddProperty(PT_INTEGER,		4, "modificationTime");
			atom->AddProperty(PT_INTEGER,		4, "timeScale");
			atom->AddProperty(PT_INTEGER,		4, "duration");
			atom->AddProperty(PT_FLOAT,			4, "rate");
			atom->AddProperty(PT_FLOAT,			2, "volume");
			atom->AddProperty(PT_BYTES,		   10, "reserved1");
			atom->AddProperty(PT_BYTES,		   36, "matrix");
			atom->AddProperty(PT_BYTES,		   24, "reserved2");
			atom->AddProperty(PT_INTEGER,		4, "nextTrackId");

		// Media header atom
		} else if (type == Mp4Utils::AtomId("mdhd")) {
			atom->AddVersionAndFlags();

			atom->AddProperty(PT_INTEGER,		4, "creationTime");
			atom->AddProperty(PT_INTEGER,		4, "modificationTime");
			atom->AddProperty(PT_INTEGER,		4, "timeScale");
			atom->AddProperty(PT_INTEGER,		4, "duration");
			atom->AddProperty(PT_INTEGER,		2, "language");
			atom->AddProperty(PT_INTEGER,		2, "quality");

		// Media information atom
		} else if (type == Mp4Utils::AtomId("minf")) {
			atom->fExpectChild = TRUE;

		// MPEG-4 audio atom
		} else if (type == Mp4Utils::AtomId("mp4a")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");
			atom->fExpectChild = TRUE;

		} else if ((type & 0xffff0000) == 0x6D730000 || type == 0x6D730002 || type == 0x6D730011) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");

		}

	} else if (*name == 'r') {
		// Root atom
		if (type == Mp4Utils::AtomId("root")) {
			atom->fExpectChild = TRUE;

		} else if (type == Mp4Utils::AtomId("raw ")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");
		}

	} else if (*name == 's') {
		// Sound media information header atom
		if (type == Mp4Utils::AtomId("smhd")) {
			atom->AddVersionAndFlags();
			atom->AddProperty(PT_INTEGER,		2, "balance");
			atom->AddProperty(PT_BYTES,			2, "reserved");

		// AMR audio atom
		} else if (type == Mp4Utils::AtomId("samr")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");	
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_BYTES,			4, "reserved2");
			atom->AddProperty(PT_INTEGER,		2, "timeScale");
			atom->AddProperty(PT_INTEGER,		2, "reserved3");
			atom->fExpectChild = TRUE;

		// Sample table atom
		} else if (type == Mp4Utils::AtomId("stbl")) {
			atom->fExpectChild = TRUE;

		// Sample description atom
		} else if (type == Mp4Utils::AtomId("stsd")) {
			atom->AddVersionAndFlags();
			atom->AddProperty(PT_INTEGER,		4, "entryCount");
			atom->fExpectChild = TRUE;

		// Time-to-sample atom
		} else if (type == Mp4Utils::AtomId("stts")) {
			atom->AddVersionAndFlags();			
			atom->AddProperty(PT_INTEGER,		4, "entryCount");
			
			Mp4TableProperty *table = new Mp4TableProperty("entries");
			table->AddColumn("sampleCount");
			table->AddColumn("sampleDelta");
			atom->fProperties.Add(table);

		// Sample-to-chunk atom
		} else if (type == Mp4Utils::AtomId("stsc")) {
			atom->AddVersionAndFlags();			
			atom->AddProperty(PT_INTEGER,		4, "entryCount");

			Mp4TableProperty *table = new Mp4TableProperty("entries");
			table->AddColumn("firstChunk");
			table->AddColumn("samplesPerChunk");
			table->AddColumn("sampleDescriptionIndex");
			atom->fProperties.Add(table);

		// Sample size atom
		} else if (type == Mp4Utils::AtomId("stsz")) {
			atom->AddVersionAndFlags();			
			atom->AddProperty(PT_INTEGER,		4, "sampleSize");
			atom->AddProperty(PT_INTEGER,		4, "entryCount");

			Mp4TableProperty *table = new Mp4TableProperty("entries");
			table->AddColumn("sampleSize");
			atom->fProperties.Add(table);

		// Sync sample atom
		} else if (type == Mp4Utils::AtomId("stss")) {
			atom->AddVersionAndFlags();			
			atom->AddProperty(PT_INTEGER,		4, "entryCount");
			
			Mp4TableProperty *table = new Mp4TableProperty("entries");
			table->AddColumn("sampleNumber");
			atom->fProperties.Add(table);

		// Chunk offset atom
		} else if (type == Mp4Utils::AtomId("stco")) {
			atom->AddVersionAndFlags();
			atom->AddProperty(PT_INTEGER,		4, "entryCount");
			
			Mp4TableProperty *table = new Mp4TableProperty("entries");
			table->AddColumn("chunkOffset");
			atom->fProperties.Add(table);
		}

	} else if (*name == 't') {
		// Track atom
		if (type == Mp4Utils::AtomId("trak")) {
			atom->fExpectChild = TRUE;

		// Track header atom
		} else if (type == Mp4Utils::AtomId("tkhd")) {
			atom->AddVersionAndFlags();
			
			atom->AddProperty(PT_INTEGER,		4, "creationTime");
			atom->AddProperty(PT_INTEGER,		4, "modificationTime");
			atom->AddProperty(PT_INTEGER,		4, "trackId");
			atom->AddProperty(PT_INTEGER,		4, "reserved1");
			atom->AddProperty(PT_INTEGER,		4, "duration");
			atom->AddProperty(PT_BYTES,			8, "reserved2");
			atom->AddProperty(PT_INTEGER,		2, "layer");
			atom->AddProperty(PT_INTEGER,		2, "alternate_group");
			atom->AddProperty(PT_FLOAT,			2, "volume");
			atom->AddProperty(PT_INTEGER,		2, "reserved3");
			atom->AddProperty(PT_BYTES,		   36, "matrix");
			atom->AddProperty(PT_FLOAT,			4, "width");
			atom->AddProperty(PT_FLOAT,			4, "height");
		}

	} else if (*name == 'u') {
		// Url atom
		if (type == Mp4Utils::AtomId("url ")) {
			atom->AddVersionAndFlags();		
			
		} else if (type == Mp4Utils::AtomId("udta")) {
			atom->AddProperty(PT_BYTES,			0, "data");

		} else if (type == Mp4Utils::AtomId("uuid")) {
			atom->AddProperty(PT_BYTES,		   16, "uuid");

		} else if (type == Mp4Utils::AtomId("ulaw")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");

		}

	} else if (*name == 'v') {
		// Video media information header atom
		if (type == Mp4Utils::AtomId("vmhd")) {
			atom->AddVersionAndFlags();
			atom->AddProperty(PT_INTEGER,		2, "graphicsmode");
			atom->AddProperty(PT_INTEGER,		2, "opcolor_red");
			atom->AddProperty(PT_INTEGER,		2, "opcolor_green");
			atom->AddProperty(PT_INTEGER,		2, "opcolor_blue");
		}
	} else {
		BYTE type_adpcm[] = {0x6D, 0x73, 0x00, 0x02, 0x00};
		BYTE type_ima_adpcm[] = {0x6D, 0x73, 0x00, 0x02, 0x00};

		if (type == Mp4Utils::AtomId((char*)type_adpcm) 
		 || type == Mp4Utils::AtomId((char*)type_ima_adpcm)
		 || type == Mp4Utils::AtomId(".mp3")) {
			atom->AddProperty(PT_BYTES,			6, "reserved1");
			atom->AddProperty(PT_INTEGER,		2, "dataReferenceIndex");
			atom->AddProperty(PT_INTEGER,		2, "version");
			atom->AddProperty(PT_INTEGER,		2, "level");
			atom->AddProperty(PT_INTEGER,		4, "vendor");
			atom->AddProperty(PT_INTEGER,		2, "channels");
			atom->AddProperty(PT_INTEGER,		2, "sampleSize");
			atom->AddProperty(PT_INTEGER,		2, "packetSize");
			atom->AddProperty(PT_INTEGER,		4, "sampleRate");
			atom->AddProperty(PT_INTEGER,		2, "reserved2");
		}
	}

	//if (atom->GetPropertyCount() <= 0) {
	//	atom->AddProperty(PT_BYTES,		0, "reserved");
	//}
	return MP4_S_OK;
}

/** 初始化这个 atom, 添加必须的子 atom 节点, 以及设置默认的属性值等. */
int Mp4Factory::InitProperties( Mp4Atom* atom, BYTE version )
{
	if (atom == NULL) {
		return 0;
	}

	LPCSTR name = atom->GetType();
	if (*name == '\0') {
		return MP4_ERR_FAILED;
	}

	static const BYTE matrix[36] = {
		0x00, 0x01, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 
		0x00, 0x01, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00, 
		0x40, 0x00, 0x00, 0x00, 
	};

	const UINT type = Mp4Utils::AtomId(name);
	if (*name == 'a') {
		if (type == Mp4Utils::AtomId("avc1")) {
			atom->AddChildAtom("avcC");

			atom->SetPropertyInt("dataReferenceIndex", 1);
			atom->SetPropertyFloat("hor", 72.0);
			atom->SetPropertyFloat("ver", 72.0);
			atom->SetPropertyInt("frameCount", 1);
			atom->SetPropertyString("compressorName", "AVC Coding");
			atom->SetPropertyInt("depth", 24);
			atom->SetPropertyInt("colorTable", 65535);

		} else if (type == Mp4Utils::AtomId("avcC")) {
			atom->SetPropertyInt("configurationVersion", 1);
			atom->SetPropertyInt("lengthSizeMinusOne", 0xFF);
			atom->SetPropertyInt("reserved2", 0x07);
		}

	} else if (*name == 'd') {
		if (type == Mp4Utils::AtomId("dinf")) {
			atom->AddChildAtom("dref");

		} else if (type == Mp4Utils::AtomId("damr")) {
			atom->SetPropertyInt("vendor", 0x6d346970);
			atom->SetPropertyInt("decoderVersion", 1);
			atom->SetPropertyInt("framesPerSample", 1);

		} else if (type == Mp4Utils::AtomId("dref")) {
			atom->AddChildAtom("url ");
			atom->SetPropertyInt("entryCount", 1);
		}

	} else if (*name == 'f') {
		if (type == Mp4Utils::AtomId("ftyp")) {
			atom->SetPropertyString("majorBrand", "mp42");
			atom->SetPropertyString("brands", "mp42isom");

		} else if (type == Mp4Utils::AtomId("free")) {
			atom->SetPropertyInt("size", 4);
		}

	} else if (*name == 'h') {
		if (type == Mp4Utils::AtomId("hdlr")) {
			atom->SetPropertyString("name", "CZ      ");
		}

	} else if (*name == 'm') {
		if (type == Mp4Utils::AtomId("moov")) {
			atom->AddChildAtom("mvhd");
			atom->AddChildAtom("iods");

		} else if (type == Mp4Utils::AtomId("mdia")) {
			atom->AddChildAtom("mdhd");
			atom->AddChildAtom("hdlr");
			atom->AddChildAtom("minf");

		} else if (type == Mp4Utils::AtomId("minf")) {
			atom->AddChildAtom("dinf");
			atom->AddChildAtom("stbl");

		} else if (type == Mp4Utils::AtomId("mp4a")) {
			atom->SetPropertyInt("dataReferenceIndex", 1);
			atom->SetPropertyInt("sampleSize", 16);
			atom->AddChildAtom("esds");

		} else if (type == Mp4Utils::AtomId("mvhd")) {
			atom->SetPropertyInt("timeScale", 1000);
			atom->SetPropertyFloat("rate", 1.0);
			atom->SetPropertyFloat("volume", 1.0);
			atom->SetPropertyInt("nextTrackId", 1);

			Mp4PropertyPtr p = atom->FindProperty("matrix");
			if (p) {
				p->SetValueBytes(matrix, 36);
			}			
		}

	} else if (*name == 'r') {
		if (type == Mp4Utils::AtomId("root")) {
			atom->AddChildAtom("ftyp");
			atom->AddChildAtom("free");
			atom->AddChildAtom("mdat");
			atom->AddChildAtom("moov");
		}

	} else if (*name == 's') {
		if (type == Mp4Utils::AtomId("stbl")) {
			atom->AddChildAtom("stsd");
			atom->AddChildAtom("stts");
			atom->AddChildAtom("stsz");

			atom->AddChildAtom("stsc");
			atom->AddChildAtom("stco");

		} else if (type == Mp4Utils::AtomId("samr")) {
			atom->SetPropertyInt("dataReferenceIndex", 1);
			atom->SetPropertyInt("channels", 2);
			atom->SetPropertyInt("sampleSize", 16);

			atom->AddChildAtom("damr");
		}

	} else if (*name == 't') {
		if (type == Mp4Utils::AtomId("trak")) {
			atom->AddChildAtom("tkhd");
			atom->AddChildAtom("mdia");			

		} else if (type == Mp4Utils::AtomId("tkhd")) {
			atom->SetPropertyInt("flags", 1);
			Mp4PropertyPtr property = atom->FindProperty("matrix");
			if (property) {
				property->SetValueBytes(matrix, 36);
			}
		}

	} else if (*name == 'u') {
		if (type == Mp4Utils::AtomId("url ")) {
			atom->SetPropertyInt("flags", 1);
		}

	} else if (*name == 'v') {
		if (type == Mp4Utils::AtomId("vmhd")) {
			atom->SetPropertyInt("flags", 1);
		}
	}

	return MP4_S_OK;
}

}
