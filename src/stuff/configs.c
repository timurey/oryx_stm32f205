/*
 * configs.c
 *
 *  Created on: 13 июня 2015 г.
 *      Author: timurtaipov
 */

#include "configs.h"
#include "ff.h"
#include "fs_port.h"

#include <stdarg.h>

static jsmn_parser *jSMNparser;
static jsmntok_t *jSMNtokens;

error_t configInit(void)
{

	jSMNparser = osAllocMem(sizeof(jsmn_parser));
	jSMNtokens = osAllocMem(sizeof(jsmntok_t) * CONFIG_JSMN_NUM_TOKENS); // a number >= total number of tokens

	if (!jSMNparser)
		return ERROR_OUT_OF_MEMORY;
	if (!jSMNtokens)
		return ERROR_OUT_OF_MEMORY;
	return NO_ERROR;
}

void configDeinit(void)
{
	osFreeMem(jSMNparser);
	osFreeMem(jSMNtokens);
}

error_t read_config(char * path, tConfigParser parser)
{
	error_t error = NO_ERROR;
	char *data;
	uint32_t fsize;
	size_t  readed;
	FsFile* file;
	if(path == NULL || parser == NULL)
		return ERROR_INVALID_PARAMETER;

	if (!fsFileExists(path))
		return ERROR_FILE_NOT_FOUND;

	error = fsGetFileSize(path, &fsize);
	if(error) return error;

	if (!fsize)
		return ERROR_INVALID_FILE_RECORD_SIZE;

	data = osAllocMem(fsize);

	if (!data) return ERROR_OUT_OF_MEMORY;

	file = fsOpenFile(path, FS_FILE_MODE_READ);

	if (!file)
	{
		osFreeMem (data);
		return ERROR_FILE_OPENING_FAILED;

	}
	error = fsReadFile (file, data, (size_t) fsize, &readed);
	if (error)
	{
		fsCloseFile(&file);
		osFreeMem (data);
		return error;

	}
	fsCloseFile(file);
	error =  parser(data, readed, jSMNparser, jSMNtokens);

	osFreeMem (data);

	return error;
}


error_t save_config (char * path, const char* fmt, ...)
{
	FsFile *file;
	va_list ap;

	if(path == NULL || fmt == NULL)
		return ERROR_INVALID_PARAMETER;

	file = fsOpenFile(path, FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);
	if (!file) 	return ERROR_FILE_OPENING_FAILED;

	va_start(ap, fmt);
	f_vprintf (file, fmt, ap);
	f_truncate(file);
	va_end(ap);

	fsCloseFile(file);
	return NO_ERROR;
}

