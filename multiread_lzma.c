/** \file multiread.c
 *
 * Created: JohnE, 2010-04-25
 */


/*
DISCLAIMER:
The author(s) of this file's contents release it into the public domain, without
express or implied warranty of any kind. You may use, modify, and redistribute
this file freely.
*/


#include <stdio.h>
#include <sys/stat.h>
#include "multiread.h"
#include "7z/LzmaDec.h"


struct LZMAFile
{
	CLzmaDec dec;
	Byte* contents;
	SizeT read;
	SizeT csize;
	UInt64 un_left;
	int last_error;
};


static void *SzAlloc(void *p, size_t size) { p = p; return malloc(size); }
static void SzFree(void *p, void *address) { p = p; free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };


struct LZMAFile* CreateLZMAFile(const char *file)
{
	FILE* infile = fopen(file, "rb");
	if (!infile)
		return 0;
	struct _stat f;
	if (_fstat(fileno(infile), &f) != 0)
	{
		fclose(infile);
		return 0;
	}
	Byte* contents = malloc(sizeof(Byte) * f.st_size);
	size_t r = fread(contents, 1, sizeof(Byte) * f.st_size, infile);
	fclose(infile);
	if (r != sizeof(Byte) * f.st_size)
	{
		free(contents);
		return 0;
	}
	UInt64 unsize = 0;
	int i = 0;
	for (; i < 8; ++i)
		unsize += (UInt64)contents[1 + i] << (i * 8);
	struct LZMAFile* lf = malloc(sizeof(struct LZMAFile));
	LzmaDec_Construct(&lf->dec);
	if (LzmaDec_Allocate(&lf->dec, contents, LZMA_PROPS_SIZE, &g_Alloc)
	 != SZ_OK)
	{
		free(lf);
		free(contents);
		return 0;
	}
	LzmaDec_Init(&lf->dec);
	lf->contents = contents;
	lf->read = LZMA_PROPS_SIZE + 8;
	lf->csize = f.st_size;
	lf->un_left = unsize;
	return lf;
}


void DestroyLZMAFile(struct LZMAFile* lf)
{
	LzmaDec_Free(&lf->dec, &g_Alloc);
	free(lf->contents);
	free(lf);
}


int ReadLZMAFile(struct LZMAFile* lf, void* buf, unsigned len)
{
	SizeT out_len = len;
	SizeT in_len = lf->csize - lf->read;
	ELzmaFinishMode fmode = LZMA_FINISH_ANY;
	if ((lf->un_left != (UInt64)(Int64)-1)
	 && out_len > lf->un_left)
	{
		out_len = lf->un_left;
		fmode = LZMA_FINISH_END;
	}
	ELzmaStatus stat;
	SRes res = LzmaDec_DecodeToBuf(&lf->dec, buf, &out_len,
	 lf->contents + lf->read, &in_len, fmode,
	 &stat);
	lf->read += in_len;
	if (lf->un_left != (UInt64)(Int64)-1)
		lf->un_left -= out_len;
	if (res != SZ_OK)
	{
		lf->last_error = res;
		return -1;
	}
	if (lf->un_left == 0
	 && stat != LZMA_STATUS_FINISHED_WITH_MARK)
	{
		lf->last_error = 12345;
		return -1;
	}
	return out_len;
}


const char* LZMAFileError(struct LZMAFile* lf, int* errnum)
{
	static char errbuf[1024];
	snprintf(errbuf, 1023, "LZMA error %d", lf->last_error);
	return errbuf;
}
