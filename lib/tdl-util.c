/*
 * This file contains useful methods for reading, writing and creating Tracked Data Log (.tdl) files
 * @author John Thomas
 */

#include <sys/types.h>
#include <sys/stat.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

#include "tdl-util.h"
#include "msg.h"
/*
 * Moves the cursor to the first data point entry.
 * This MUST be called before any calls to tdl_read.
 * Additionally, this will store the name of the tracked
 * object that the file contatins to the given pointer.
 *
 * @param int fd - a file descripter pointing to the file.
 *		  char** name - a pointer to the char* where the name should be stored.
 *						if this is null, the name will be ignored.
 */
int tdl_prepare(int fd, char** name)
{
#ifdef _WIN32
	msg(MSG_ERROR, "This function is not defined on Windows.");
	return -1;
#else

	//Seek to the begining of the file.
	lseek(fd, 0, SEEK_SET);
	
	if(tdl_validate(fd))
	{
		char c = 0;
		char* tmpName = (char*)malloc(sizeof(char) * 33);
		int i = 0, bRead = 0;
		/*
		  Read until we find a NULL char that indicates the end of the name
		  even if we reach the char limit, we still need to keep reading just
		  in case a name longer than 32 chars was written to the file some how.
		  The extra chars should be ignored, this is simple to set the cursor
		  for further reads.
		*/
		while((bRead = read(fd, &c, 1)) > 0)
		{
			if(i < 33)
			{
				tmpName[i++] = c;
			}
			
			if(c == 0) break;
		}
		
		//error check for the read.
		if(bRead < 1)
		{
			perror("Reading object name failed");
		}
		
		//If the specified pointer isn't null, store to it.
		if(name != NULL)
		{
			name[0] = tmpName;
		}	
	}
	else
	{
		//return -1 if the file is invalid.
		return -1;
	}
	
	//Everthing went fine, so return 1.
	return 1;
#endif
}

/*
 * Creates a new empty tdl file. This will setup 
 *
 * Proper header: 219  84  68  76  13  10  26  10
 * ASCII  header: INV   T   D   L  \r  \n \032 \n
 * We use this combination of invalid ascii and different returns to make
 * the file not openable with text editors since it is a binary file.
 *
 * @param char* path - the path to the file and the file name. 
 * 				       ".tdl" will be appended to the file name if
 *					   it is not specified in the path.
 *		  char* name - the name of the object that the file contains.
 *					   There is a limit of 32 chars for the object name,
 *					   anything longer will be truncated.
 *
 * @return int - a file descripter pointing to the newly created file.
 */
int tdl_create(const char* path, const char* name)
{
#ifdef _WIN32
	msg(MSG_ERROR, "This function is not defined on Windows.");
	return -1;
#else

	int pathLen = strlen(path);
	int nameLen = strlen(name);
	nameLen = nameLen > 32 ? 32 : nameLen;

	char buff[1024];
	//Check the last 4 chars in the path to see if they are .tdl.
	if(pathLen < 4 || strncmp(path + pathLen - 4, ".tdl", 4) != 0)
	{
		buff[0] = '\0';
		strncat(buff, path, pathLen);
		strncat(buff, ".tdl", 4);
		path = buff;
		pathLen += 4;
	}

	int fd = -1;
	if((fd = open(path, O_CREAT | O_EXCL | O_RDWR , S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) < 0)
	{
		perror("File creation failed");
		return fd;
	}
	
	unsigned char header[9];
	header[0] = 219;//INV ascii char for identifier
	header[1] = 84;//T
	header[2] = 68;//D
	header[3] = 76;//L
	header[4] = 13;//\r
	header[5] = 10;//\n
	header[6] = 26;//\032
	header[7] = 10;//\n
	header[8] = 0;//NULL term
	if(write(fd, header, 9) < 9)
	{
		perror("Writing header failed");
		return fd;
	}
	
	//We +1 to the name length so that the term char is also written.
	if(write(fd, name, nameLen+1) < nameLen+1)
	{
		perror("Writing object name failed");
		return fd;
	}
	
	return fd;
#endif
}
/*
 * Returns the next tracked point in the file.
 * If the end of the file is reached, the file cursor will be
 * moved back to the beginning of the file.
 *
 * @param int fd - a file descripter pointing to the file.
 *		  float* pos - the array that the position will be stored. Should be length 3.
 *		  float* orient - the array that the orientation will be stored. Should be length 16.
 *
 * @return int - -1 if the file could not be read.
 * 				  0 if the operation was normal.
 *				  1 if the end of the file was reached and the cursor was reset.
 *
 */
int tdl_read(int fd, float pos[3], float orient[9])
{
#ifdef _WIN32
	msg(MSG_ERROR, "This function is not defined on Windows.");
	return -1;
#else

	int posSize = 3*(signed int)sizeof(float);
	int orientSize = 9*(signed int)sizeof(float);
	
	int readVal = 0;
	if((readVal = read(fd, pos, posSize)) < posSize)
	{
		if(readVal == 0){
			//EOF
			return 1;
		}
		else
		{
			perror("Reading position failed");
			return -1;
		}
	}
	
	if((readVal = read(fd, orient, orientSize)) < orientSize)
	{
		if(readVal == 0){
			//EOF
			return 1;
		}
		else
		{
			perror("Reading position failed");
			return -1;
		}
	}
	
	return 0;
#endif
}

/*
 * Writes the position and orientation properly formated to a file.
 *
 * @param int fd - the file to write to
 * 		  float* pos - the position array 
 * 		  float* orient - the orientation array
 *
 */
void tdl_write(int fd, float pos[3], float orient[9])
{
#ifdef _WIN32
	msg(MSG_ERROR, "This function is not defined on Windows.");
#else

	int posSize = 3*(signed int)sizeof(float);
	int orientSize = 9*(signed int)sizeof(float);
	if(write(fd, pos, posSize) < posSize)
	{
		perror("Writing position failed");
	}
	
	if(write(fd, orient, orientSize) < orientSize)
	{
		perror("Writing orientation failed");
	}
#endif
}

/*
 * Checks the headers of a file to make sure it is a proper tdl file.
 * Note: the position of the cursor in the file MUST be at the CUR_START
 *
 * @param int fd - a file descripter pointing to the file.
 *
 * @return (C)int - 0 - the file is not valid.
 *					1 - the file is valid.
 *
 *		   (C++)bool - whether or not the file is valid
 */
#ifdef __cplusplus
bool tdl_validate(int fd)
#else
int tdl_validate(int fd)
#endif
{
#ifdef _WIN32
	msg(MSG_ERROR, "This function is not defined on Windows.");
	return 0;
#else
	unsigned char buff[9];
	int valid = 0;
	ssize_t lenRead = read(fd, buff, 9);
	if(lenRead == 9)
	{
		valid = buff[0] == 219 && buff[1] == 84 &&
			    buff[2] == 68 && buff[3] == 76 && 
			    buff[4] == 13 && buff[5] == 10 && 
			    buff[6] == 26 && buff[7] == 10 && 
			    buff[8] == 0;
	}
	
#ifdef __cplusplus
	return (bool)valid;
#else
	return valid;
#endif

#endif
}


