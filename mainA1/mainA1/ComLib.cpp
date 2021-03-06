#include "ComLib.h"

using namespace std;

ComLib::ComLib()
{
	buffSize = 0;
}

ComLib::ComLib(const std::string & secret, const size_t & _buffSize, TYPE type)
{
	this->buffSize = (_buffSize * (1<<20)) - (2 * sizeof(size_t));
	memoryName = secret;
	this->type = type;

	//Check if first
	fileMap.hFileMap = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		(DWORD)0,
		_buffSize * (1<<20),
		(LPCSTR)memoryName.c_str()
	);

	if (fileMap.hFileMap == NULL)
	{
		printf("File map couldn't be created.\n\n");
	}

	fileMap.mData = MapViewOfFile(fileMap.hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	head = (size_t*)fileMap.mData;
	tail = head + 1;
	base = (char*)(head + 2);

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
	}
	else
	{
		*head = 0;
		*tail = 0;
	}
}

ComLib::~ComLib()
{
	UnmapViewOfFile((LPCVOID)fileMap.mData);
	CloseHandle(fileMap.hFileMap);
}

bool ComLib::write(const void * msg, const size_t length)
{
	if (length < buffSize / 2)
	{
		size_t localTail = *tail;

		//If (h < t) -> t - h
		//If (t < h) -> (sizeMem - h) + t
		if (*head < localTail)
		{
			freeMemory = localTail - *head;
		}
		else
		{
			freeMemory = buffSize - *head;
		}

		if ((length + sizeof(HEADER) * 3) < freeMemory)
		{
			h = { NORMAL, length };
			memcpy(base + *head, &h, sizeof(HEADER));
			
			memcpy(base + *head + sizeof(HEADER), msg, length);
			
			*head = *head + length + sizeof(HEADER);
			return true;
		}
		else
		{
			if (localTail != 0 && localTail <= *head)
			{
				h = { DUMMY, length };

				*(HEADER*)(base + *head) = h;
				*head = 0;
				return false;
			}
			else
			{
				printf("There's not enough memory to write\n");
				return false;
			}
		}
	}
	
	return false;
}

bool ComLib::read(char * msg, size_t & length)
{
	size_t localHead = *head;

	if (localHead == *tail)
	{
		return false;
	}

	HEADER h = { };
	h = *((HEADER*)(base + (*tail)));
	length = h.length;
		
	if (h.type == DUMMY)
	{	
		*tail = 0;
		return false;
	}
	else
	{
		memcpy(msg, (char*)(base + *tail + sizeof(HEADER)), length);

		*tail = *tail + length + sizeof(HEADER);
		return true;
	}
	return false;
}

size_t ComLib::nextSize()
{
	return size_t();
}
