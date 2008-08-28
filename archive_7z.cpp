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
#include "7z/7zCrc.h"
#include "7z/Archive/7z/7zIn.h"
#include "7z/Archive/7z/7zExtract.h"
#include "archive_base.h"
}
#define WIN32_LEAN_AND_MEAN
#include <windows.h>


extern "C"
{

typedef struct
{
	ISzInStream sz_stream;
	FILE* filep;
} FileInStream;

static SZ_RESULT SzFileReadImp
 (void *object,
  void *buffer,
  size_t size,
  size_t *processedSize)
{
	FileInStream* f = (FileInStream*)object;
	int read = fread(buffer, 1, size, f->filep);
	if (processedSize)
		*processedSize = read;
	else if (read <= 0)
		return SZE_FAIL;
	return SZ_OK;
}

static SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
	FileInStream* f = (FileInStream*)object;
	int res = fseek(f->filep, (long)pos, SEEK_SET);
	if (res == 0)
		return SZ_OK;
	return SZE_FAIL;
}

} // extern "C"


extern int num_files_in_cur_op;
extern int cur_file_in_op_index;


static void SzArDbExDeleter(CArchiveDatabaseEx* db)
{
	SzArDbExFree(db, SzFree);
}


static void OutBufferDeleter(Byte* ob)
{
	if (ob)
		SzFree(ob);
}


int un7z
 (const char* file,
  const char* base,
  int (*before_entry_callback)(const char*, int),
  void (*create_callback)(const char*, int))
{
	FileInStream instream;
	instream.sz_stream.Read = SzFileReadImp;
	instream.sz_stream.Seek = SzFileSeekImp;
	instream.filep = fopen(file, "rb");
	if (!instream.filep)
	{
		archive_seterror("failed to open file '%s' for reading", file);
		return -1;
	}
	RefType< FILE >::Ref fcloser(instream.filep, fclose);

	CrcGenerateTable();

	CArchiveDatabaseEx db;
	SzArDbExInit(&db);
	RefType< CArchiveDatabaseEx >::Ref db_deleter(&db, SzArDbExDeleter);

	ISzAlloc allocImp, allocTempImp;
	allocImp.Alloc = SzAlloc;
	allocImp.Free = SzFree;
	allocTempImp.Alloc = SzAllocTemp;
	allocTempImp.Free = SzFreeTemp;

	if (SzArchiveOpen(&instream.sz_stream, &db, &allocImp, &allocTempImp)
	 != SZ_OK)
	{
		archive_seterror("7zlib couldn't open '%s' as a 7z archive", file);
		return -1;
	}

	num_files_in_cur_op = db.Database.NumFiles;

	UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
	Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
	size_t outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */
	RefType< Byte >::Ref outbuf_deleter(outBuffer, OutBufferDeleter);
	for (unsigned i = 0; i < db.Database.NumFiles; i++)
	{
		cur_file_in_op_index = i;
		CFileItem* cur_file = &db.Database.Files[i];
		if (before_entry_callback)
		{
			int cbres = before_entry_callback(cur_file->Name, cur_file->IsDirectory);
			if (cbres != 0)
			{
				archive_seterror("7zlib failed to unzip '%s': cancelled from "
				 "callback", cur_file->Name);
				return cbres;
			}
		}
		std::string out_path = std::string(base) + "/" + cur_file->Name;
		if (cur_file->IsDirectory)
		{
			if (!makedir(base, cur_file->Name))
			{
				archive_seterror("couldn't create directory '%s'",
				 out_path.c_str());
				return -1;
			}
		}
		else
		{
			size_t offset, outSizeProcessed;
			SZ_RESULT res = SzExtract(&instream.sz_stream, &db, i, &blockIndex,
			 &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &allocImp,
			 &allocTempImp);
			if (res == SZE_CRC_ERROR)
			{
				archive_seterror("bad CRC for file '%s' in '%s'",
				 cur_file->Name, file);
				return -1;
			}
			else if (res != SZ_OK)
			{
				archive_seterror("7zlib failed to unpack file '%s' in '%s'",
				 cur_file->Name, file);
				return -1;
			}
			bool was_created = false;
			if (chmod(out_path.c_str(), _S_IREAD|_S_IWRITE) != 0)
				was_created = true;
			FILE* outfile = fopen(out_path.c_str(), "wb");
			if (!outfile)
			{
				size_t p = std::string(cur_file->Name).find_last_of("/\\", std::string::npos);
				if (p != std::string::npos)
				{
					makedir(base, std::string(cur_file->Name).substr(0, p).c_str());
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
		if (cur_file->AreAttributesDefined)
			SetFileAttributes(out_path.c_str(), cur_file->Attributes);
		create_callback(cur_file->Name, cur_file->IsDirectory ? 1 : 0);
	}

	return 0;
}

