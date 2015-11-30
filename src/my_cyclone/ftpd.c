/*
 * ftpd.c
 *
 *  Created on: 25 апр. 2015 г.
 *      Author: timurtaipov
 */

#include "ftpd.h"

static FtpServerSettings ftpServerSettings;

error_t ftpdConfigure (void)
{
	error_t error;


	//Get default settings
	ftpServerGetDefaultSettings(&ftpServerSettings);
	//Bind FTP server to the desired interface
	ftpServerSettings.interface = &netInterface[0];
	//Listen to port 21
	ftpServerSettings.port = FTP_PORT;
	//Root directory
	strcpy(ftpServerSettings.rootDir, "/");
	//User verification callback function
	ftpServerSettings.checkUserCallback = ftpServerCheckUserCallback;
	//Password verification callback function
	ftpServerSettings.checkPasswordCallback = ftpServerCheckPasswordCallback;
	//Callback used to retrieve file permissions
	ftpServerSettings.getFilePermCallback = ftpServerGetFilePermCallback;


	return error;
}

error_t ftpdStart (void)
{
   error_t error;
   //FTP server initialization
   error = ftpServerInit(&ftpServerContext, &ftpServerSettings);
   //Failed to initialize FTP server?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to initialize FTP server!\r\n");
   }
   //Start FTP server
   error = ftpServerStart(&ftpServerContext);
   //Failed to start FTP server?
   if(error)
   {
      //Debug message
      TRACE_ERROR("Failed to start FTP server!\r\n");
   }
   return error;
}
/**
 * @brief User verification callback function
 * @param[in] connection Handle referencing a client connection
 * @param[in] user NULL-terminated string that contains the user name
 * @return Access status (FTP_ACCESS_ALLOWED, FTP_ACCESS_DENIED or FTP_PASSWORD_REQUIRED)
 **/

uint_t ftpServerCheckUserCallback(FtpClientConnection *connection,
		const char_t *user)
{
	//Debug message
	TRACE_INFO("FTP server: User verification\r\n");

	//Manage authentication policy
	if(!strcmp(user, "anonymous"))
	{
		return FTP_ACCESS_ALLOWED;
	}
	else if(!strcmp(user, "admin"))
	{
		return FTP_PASSWORD_REQUIRED;
	}
	else
	{
		return FTP_ACCESS_DENIED;
	}
}


/**
 * @brief Password verification callback function
 * @param[in] connection Handle referencing a client connection
 * @param[in] user NULL-terminated string that contains the user name
 * @param[in] password NULL-terminated string that contains the corresponding password
 * @return Access status (FTP_ACCESS_ALLOWED or FTP_ACCESS_DENIED)
 **/

uint_t ftpServerCheckPasswordCallback(FtpClientConnection *connection,
		const char_t *user, const char_t *password)
{
	//Debug message
	TRACE_INFO("FTP server: Password verification\r\n");

	//Verify password
	if(!strcmp(user, "admin") && !strcmp(password, "admin"))
	{
		return FTP_ACCESS_ALLOWED;
	}
	else
	{
		return FTP_ACCESS_DENIED;
	}
}


/**
 * @brief Callback used to retrieve file permissions
 * @param[in] connection Handle referencing a client connection
 * @param[in] user NULL-terminated string that contains the user name
 * @param[in] path Canonical path of the file
 * @return Permissions for the specified file
 **/

uint_t ftpServerGetFilePermCallback(FtpClientConnection *connection,
		const char_t *user, const char_t *path)
{
	uint_t perm;

	//Debug message
	TRACE_INFO("FTP server: Checking access rights for %s\r\n", path);

	//Manage access rights
	if(!strcmp(user, "anonymous"))
	{
		//Allow read/write access to temp directory
		if(pathMatch(path, "/temp/*"))
			perm = FTP_FILE_PERM_LIST | FTP_FILE_PERM_READ | FTP_FILE_PERM_WRITE;
		//Allow read access only to other directories
		else
			perm = FTP_FILE_PERM_LIST | FTP_FILE_PERM_READ;
	}
	else if(!strcmp(user, "admin"))
	{
		//Allow read/write access
		perm = FTP_FILE_PERM_LIST | FTP_FILE_PERM_READ | FTP_FILE_PERM_WRITE;
	}
	else
	{
		//Deny access
		perm = 0;
	}

	//Return the relevant permissions
	return perm;
}
