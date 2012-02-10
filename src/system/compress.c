/*
Copyright (C) 2009-2012 Parallel Realities

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA.
*/

#include "../headers.h"

#include "error.h"

static unsigned char *decompressFileOld(char *);

void compressFile(char *sourceName)
{
	int read, result;
	unsigned char *source, *dest;
	int fileSize, ensuredSize;
	unsigned long compressedSize;
	FILE *fp;

	fp = fopen(sourceName, "rb");

	fseek(fp, 0L, SEEK_END);

	fileSize = ftell(fp);

	ensuredSize = fileSize * 1.01 + 12;

	fseek(fp, 0L, SEEK_SET);

	source = malloc(fileSize * sizeof(unsigned char));

	if (source == NULL)
	{
		showErrorAndExit("Failed to allocate %ld bytes to compress save file", fileSize * sizeof(unsigned char));
	}

	dest = malloc(ensuredSize * sizeof(unsigned char));

	if (dest == NULL)
	{
		showErrorAndExit("Failed to allocate %ld bytes to compress save file", ensuredSize * sizeof(unsigned char));
	}

	compressedSize = ensuredSize;

	read = fread(source, fileSize, 1, fp);

	fclose(fp);

	result = compress2(dest, &compressedSize, source, fileSize, 9);
	
	if (result != Z_OK)
	{
		switch (result)
		{
			case Z_BUF_ERROR:
				showErrorAndExit("Compression of file %s failed. Buffer too small", sourceName);
			break;
			
			case Z_MEM_ERROR:
				showErrorAndExit("Compression of file %s failed. Insufficient memory", sourceName);
			break;
			
			default:
				showErrorAndExit("Compression of file %s failed. Stream size incorrectly defined", sourceName);
			break;
		}
	}

	fp = fopen(sourceName, "wb");

	result = fwrite(&fileSize, sizeof(int), 1, fp);
	
	if (result != 1)
	{
		showErrorAndExit("Failed to write original filesize: %s", strerror(errno));
	}

	result = fwrite(dest, compressedSize, 1, fp);
	
	if (result != 1)
	{
		showErrorAndExit("Failed to write compressed data: %s", strerror(errno));
	}

	free(source);

	free(dest);

	fclose(fp);
}

unsigned char *decompressFile(char *sourceName)
{
	unsigned char *source, *dest;
	int compressedSize, int32;
	unsigned long fileSize;
	FILE *fp;
	int read, result;

	fp = fopen(sourceName, "rb");

	if (fp == NULL)
	{
		showErrorAndExit("Could not open save file %s", sourceName);
	}

	fseek(fp, 0L, SEEK_END);

	compressedSize = ftell(fp);

	compressedSize -= sizeof(int);

	fseek(fp, 0L, SEEK_SET);

	read = fread(&int32, sizeof(int), 1, fp);

	fileSize = int32;
	
	if (read != 1)
	{
		printf("1. New decompression method failed, trying old method...\n");

		return decompressFileOld(sourceName);
	}

	source = malloc(compressedSize * sizeof(unsigned char));

	if (source == NULL)
	{
		printf("2. New decompression method failed, trying old method...\n");

		return decompressFileOld(sourceName);
	}

	dest = malloc((fileSize + 1) * sizeof(unsigned char));

	if (dest == NULL)
	{
		free(source);

		printf("3. New decompression method failed, trying old method...\n");
		
		return decompressFileOld(sourceName);
	}

	read = fread(source, compressedSize, 1, fp);
	
	if (read != 1)
	{
		free(source);
		
		free(dest);
		
		printf("4.New decompression method failed, trying old method...\n");
		
		return decompressFileOld(sourceName);
	}

	result = uncompress(dest, &fileSize, source, compressedSize);

	dest[fileSize] = '\0';

	fclose(fp);

	free(source);
	
	if (result != Z_OK)
	{
		if (dest != NULL)
		{
			free(dest);
		}
		
		printf("5. New decompression method failed, trying old method...\n");
		
		return decompressFileOld(sourceName);
	}

	return dest;
}

static unsigned char *decompressFileOld(char *sourceName)
{
	unsigned char *source, *dest;
	uint64_t long64;
	unsigned long compressedSize, fileSize;
	FILE *fp;
	int read, result;

	fp = fopen(sourceName, "rb");

	if (fp == NULL)
	{
		showErrorAndExit("Could not open save file %s", sourceName);
	}

	fseek(fp, 0L, SEEK_END);

	compressedSize = ftell(fp);

	compressedSize -= sizeof(long64);

	fseek(fp, 0L, SEEK_SET);

	read = fread(&long64, sizeof(long64), 1, fp);
	
	fileSize = long64;
	
	if (read != 1)
	{
		showErrorAndExit("Failed to read original filesize: %s", strerror(errno));
	}

	source = malloc(compressedSize * sizeof(unsigned char));

	if (source == NULL)
	{
		showErrorAndExit("Failed to allocate %ld bytes to decompress save file", compressedSize * sizeof(unsigned char));
	}

	dest = malloc((fileSize + 1) * sizeof(unsigned char));

	if (dest == NULL)
	{
		showErrorAndExit("Failed to allocate %ld bytes to decompress save file", (fileSize + 1) * sizeof(unsigned char));
	}

	read = fread(source, compressedSize, 1, fp);
	
	if (read != 1)
	{
		showErrorAndExit("Failed to read compressed data: %s", strerror(errno));
	}

	result = uncompress(dest, &fileSize, source, compressedSize);

	dest[fileSize] = '\0';

	fclose(fp);

	free(source);
	
	if (result != Z_OK)
	{
		switch (result)
		{
			case Z_BUF_ERROR:
				showErrorAndExit("Decompression of file %s failed. Buffer too small", sourceName);
			break;
			
			case Z_MEM_ERROR:
				showErrorAndExit("Decompression of file %s failed. Insufficient memory", sourceName);
			break;
			
			default:
				showErrorAndExit("Decompression of file %s failed. Data is corrupt", sourceName);
			break;
		}
	}

	return dest;
}
