/** \file archive_7z.cpp
 *
 * Created: JohnE, 2008-07-13
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#include <string>
#include <cstdio>
#include "ref.hpp"
extern "C" {
#include <sys/stat.h>
#include "7z/7z.h"
#include "7z/7zFile.h"
#include "7z/7zCrc.h"
#include "7z/Util/7z/7zAlloc.h"
#include "archive_base.h"
}
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


extern int num_files_in_cur_op;
extern int cur_file_in_op_index;


static void FileCloser(CFileInStream* instream)
{
	File_Close(&instream->file);
}

struct DBDeleter
{
	ISzAlloc* alloc_imp;
	DBDeleter(ISzAlloc* aimp) : alloc_imp(aimp) {}
	void operator () (CSzArEx* db) { SzArEx_Free(db, alloc_imp); }
};

struct OutBufDeleter
{
	ISzAlloc* alloc_imp;
	OutBufDeleter(ISzAlloc* aimp) : alloc_imp(aimp) {}
	void operator () (Byte** outbuf)
	{
		if (*outbuf)
			IAlloc_Free(alloc_imp, *outbuf);
	}
};

void UInt16ArrayDeleter(UInt16* del)
{
	delete[] del;
}


int un7z
 (const char* file,
  const char* base,
  int (*before_entry_callback)(const char*, int),
  void (*create_callback)(const char*, int))
{
	CFileInStream instream;
	if (InFile_Open(&instream.file, file))
	{
		archive_seterror("failed to open file '%s' for reading", file);
		return -1;
	}
	RefType< CFileInStream >::Ref fcloser(&instream, FileCloser);
	FileInStream_CreateVTable(&instream);

	CLookToRead lookstream;
	LookToRead_CreateVTable(&lookstream, False);
	lookstream.realStream = &instream.s;
	LookToRead_Init(&lookstream);

	CrcGenerateTable();

	ISzAlloc allocImp, allocTempImp;
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	CSzArEx db;
	SzArEx_Init(&db);

	if (SzArEx_Open(&db, &lookstream.s, &allocImp, &allocTempImp) != SZ_OK)
	{
		archive_seterror("7zlib couldn't open '%s' as a 7z archive", file);
		return -1;
	}
	RefType< CSzArEx >::Ref db_deleter(&db, DBDeleter(&allocImp));

	num_files_in_cur_op = db.db.NumFiles;

	UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
	Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
	size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
	RefType< Byte* >::Ref outbuf_deleter(&outBuffer, OutBufDeleter(&allocImp));

	for (unsigned i = 0; i < db.db.NumFiles; ++i)
	{
		cur_file_in_op_index = i;
		const CSzFileItem* cur_file = &db.db.Files[i];

		size_t len16 = SzArEx_GetFileNameUtf16(&db, i, 0);
		UInt16 name16[len16 + 1];
		SzArEx_GetFileNameUtf16(&db, i, name16);
		char name8[len16 * 3 + 100];
		char defaultChar = '_';
		BOOL defUsed;
		int numChars = WideCharToMultiByte(CP_OEMCP, 0, (const WCHAR*)name16,
		 len16, name8, len16 * 3 + 100, &defaultChar, &defUsed);
		name8[numChars] = 0;

		if (before_entry_callback)
		{
			int cbres = before_entry_callback(name8, cur_file->IsDir);
			if (cbres != 0)
			{
				archive_seterror("7zlib failed to unzip '%s': cancelled from "
				 "callback", name8);
				return cbres;
			}
		}

		std::string out_path = std::string(base) + "/" + name8;
		if (cur_file->IsDir)
		{
			if (!makedir(base, name8))
			{
				archive_seterror("couldn't create directory '%s'",
				 out_path.c_str());
				return -1;
			}
		}
		else
		{
			size_t offset, outSizeProcessed;
			SRes res = SzArEx_Extract(&db, &lookstream.s, i, &blockIndex,
			 &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp,
			 &allocTempImp);
			if (res == SZ_ERROR_CRC)
			{
				archive_seterror("bad CRC for file '%s' in '%s'", name8, file);
				return -1;
			}
			else if (res != SZ_OK)
			{
				archive_seterror("7zlib failed to unpack file '%s' in '%s'",
				 name8, file);
				return -1;
			}
			bool was_created = false;
			if (chmod(out_path.c_str(), _S_IREAD|_S_IWRITE) != 0)
				was_created = true;
			FILE* outfile = fopen(out_path.c_str(), "wb");
			if (!outfile)
			{
				size_t p = std::string(name8).find_last_of("/\\", std::string::npos);
				if (p != std::string::npos)
				{
					makedir(base, std::string(name8).substr(0, p).c_str());
					outfile = fopen(out_path.c_str(), "wb");
				}
			}
			if (!outfile)
			{
				archive_seterror("couldn't open '%s' for writing",
				 out_path.c_str());
				return -1;
			}
			int written = fwrite(outBuffer + offset, 1, outSizeProcessed,
			 outfile);
			fclose(outfile);
			if (written <= 0
			 || static_cast< size_t >(written) != outSizeProcessed)
			{
				remove(out_path.c_str());
				archive_seterror("failed to write %d bytes to '%s'",
				 outSizeProcessed, out_path.c_str());
				return -1;
			}
		}
		if (cur_file->AttribDefined)
			SetFileAttributes(out_path.c_str(), cur_file->Attrib);
		create_callback(name8, cur_file->IsDir ? 1 : 0);
	}

	return 0;
}

