#include <Windows.h>
#include "Buffer.h"
#include <exception>
#include <iostream>
#include <stdio.h>
using namespace std;

HANDLE hFull, hEmpty, hFile;
LPCSTR lpFileName = "$SharedFileName$";
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

enum ConsoleColor 
{
	Black = 0, Blue, Green, Cyan, Red, Magenta, Brown, LightGray, DarkGray, LightBlue, LightGreen, LightCyan, LightRed, LightMagenta, Yellow, White
};

LPVOID GetFileMapping(HANDLE hFileMapping)
{
	LPVOID lpFileMap = MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (lpFileMap == 0)
		throw std::exception("Open operation was failed");
	return lpFileMap;
}

void OpenFileMap(HANDLE &hFileMapping, LPVOID &lpFileMap)
{
	hFileMapping = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, lpFileName);
	lpFileMap = GetFileMapping(hFileMapping);
}

void CreateFileMap(HANDLE &hFileMapping, LPVOID &lpFileMap) 
{
	hFileMapping = CreateFileMapping((HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE, 0, 100, lpFileName);
	lpFileMap = GetFileMapping(hFileMapping);
}

DWORD WINAPI ConsumerThread(void *params) 
{
	int i = *((int*)params);
	HANDLE hFileMapping;
	LPVOID lpFileMap;
	OpenFileMap(hFileMapping, lpFileMap);
	int value;
	int changed = -1;
	while (true) 
	{
		Sleep(rand() % 1000);
		WaitForSingleObject(hFull, INFINITE);
		WaitForSingleObject(hFile, INFINITE);
		Buffer buffer = *((Buffer*)lpFileMap);
		for (int i = 0; i < buffer.GetSize(); i++) {
			if (buffer[i] != 0)
			{
				changed = i;
				value = buffer[i];
				buffer[i] = 0;
				cout << "Consumer " << i << ": " << value << endl;
				break;
			}
		}
		cout << "Status bufer:  ";
		for (int i = 0; i < buffer.GetSize(); i++) 
		{
			if (changed != -1 && changed == i) 
			{
				SetConsoleTextAttribute(hConsole, (WORD)((Black << 4) | Red));
				cout << " | " << buffer[i] << " | ";
				SetConsoleTextAttribute(hConsole, (WORD)((Black << 4) | White));
			}
			else cout << " | " << buffer[i] << " | ";
		}
		cout << "\n";
		*((Buffer*)lpFileMap) = buffer;
		ReleaseSemaphore(hFile, 1, NULL);
		ReleaseSemaphore(hEmpty, 1, NULL);
	}
	return 0;
}

static DWORD WINAPI ProducerThread(void *params) 
{
	int i = *((int*)params);
	HANDLE hFileMapping;
	LPVOID lpFileMap;
	OpenFileMap(hFileMapping, lpFileMap);
	int value;
	int changed = -1;
	while (true)
	{
		Sleep(rand() % 1000);
		value = (rand() % 9) + 1;
		WaitForSingleObject(hEmpty, INFINITE);
		WaitForSingleObject(hFile, INFINITE);
		Buffer buffer = *((Buffer*)lpFileMap);
		for (int i = 0; i < buffer.GetSize(); i++)
		{
			if (buffer[i] == 0)
			{
				buffer[i] = value;
				changed = i;
				cout << "Producer " << i << ": " << value << endl;
				break;
			}
		}
		cout << "Status bufer:  ";
		for (int i = 0; i < buffer.GetSize(); i++) 
		{
			if (changed != -1 && changed == i) 
			{
				SetConsoleTextAttribute(hConsole, (WORD)((Black << 4) | Green));
				cout << " | " << buffer[i] << " | ";
				SetConsoleTextAttribute(hConsole, (WORD)((Black << 4) | White));
			}
			else cout << " | " << buffer[i] << " | ";
		}
		cout << "\n";
		*((Buffer*)lpFileMap) = buffer;
		ReleaseSemaphore(hFile, 1, NULL);
		ReleaseSemaphore(hFull, 1, NULL);
	}
	return 0;
}

void RunThreads(HANDLE hThreads[], int consumers, int producers)
{
	int length = producers + consumers;
	for (int i = 0; i < producers; i++)
	{
		hThreads[i] = CreateThread(0, 0, ProducerThread, (void*)new int(i), 0, 0);
	}	
	for (int i = producers; i < length; i++)
	{
		hThreads[i] = CreateThread(0, 0, ConsumerThread, (void*)new int(i - producers), 0, 0);
	}
}

int main()
{
	const int producers = 3;
	const int consumers = 2;
	const int size = 4;
	const int length = producers + consumers;

	hFile = CreateSemaphore(NULL, 0, 1, NULL);
	hFull = CreateSemaphore(NULL, 1, 1, NULL);
	hEmpty = CreateSemaphore(NULL, 1, 1, NULL);

	HANDLE hThreads[length];
	LPVOID lpFileMap;
	HANDLE hFileMapping;
	CreateFileMap(hFileMapping, lpFileMap);

	int arr[size] = { 1, 1, 1, 1 };
	Buffer buffer(arr, size);
	*((Buffer*)lpFileMap) = buffer;

	RunThreads(hThreads, consumers, producers);
	WaitForMultipleObjects(producers + consumers, hThreads, true, 8000);
	for (int i = 0; i < length; i++)
	{
		TerminateThread(hThreads[i], 0);
	}

	CloseHandle(hFile);
	CloseHandle(hFull);
	CloseHandle(hEmpty);

	UnmapViewOfFile(lpFileMap);
	CloseHandle(hFileMapping);


	system("pause");
	return 0;
}
