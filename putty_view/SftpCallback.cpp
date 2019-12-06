#include "SftpCallback.h"
#include <memory>
#include <Windows.h>
#include <iostream>
#include <tchar.h>

#ifdef __cplusplus
extern "C" {
#include <stdint.h>
#include "../psftp/psftp.h"
}
#endif

#define ARGV_P "-P"
#define ARGV_L "-l"
#define ARGV_PW "-pw"
#define ARGV_SSHLOGFILE "-sshlog"



void CreateConsole()
{
	if (!AllocConsole()) {
		// Add some error handling here.
		// You can call GetLastError() to get more info about the error.
		return;
	}

	// std::cout, std::clog, std::cerr, std::cin
	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(_T("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(_T("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();
	//::SetParent(GetConsoleWindow(), hWnd);
}


void Start_Sftp(char *host, int port, char *name,char *pass, char *logfile)
{
	// -P 22 -l fyx -pw fyx999 -sshlog ssa.log 192.168.128.222
	//int argc = 9;
	int argc = 8;
	char *argv[8];
	for (int i = 0; i < argc; i++)
	{
		argv[i] = new char[256];
		memset(argv[i], 0,  256);
	}
	
	memcpy(argv[1], ARGV_P, strlen(ARGV_P));
	itoa(port, argv[2], 10);
	memcpy(argv[3], ARGV_L, strlen(ARGV_L));
	memcpy(argv[4], name, strlen(name));
	memcpy(argv[5], ARGV_PW, strlen(ARGV_PW));
	memcpy(argv[6], pass, strlen(pass));
	memcpy(argv[7], host, strlen(host));
	//memcpy(argv[7], ARGV_SSHLOGFILE, strlen(ARGV_SSHLOGFILE));
	//memcpy(argv[8], logfile, strlen(logfile));
	//CreateConsole();

	//psftp_main(argc, (char **)argv);
}
