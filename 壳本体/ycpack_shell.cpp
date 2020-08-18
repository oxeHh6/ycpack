// ycpack_shell.cpp : Defines the entry point for the console application.
//

#include "StdAfx.h"
#include "head.h"
#define  MY_DEBUG
char g_NameOfNewSectionHeader[] = {'Y', 'U', 'C', 'H', 'U'};

int main()
{
    //--------------------------------------���ܹ���--------------------------------------
    LPVOID pFileBuffer = NULL;
    PIMAGE_DOS_HEADER pDosHeader = NULL;
    PIMAGE_NT_HEADERS32 pNTHeader = NULL;
    PIMAGE_SECTION_HEADER pSectionHeader = NULL;
    DWORD file_size = 0;

    //��ȡ��ǰ��������·��
    char FilePathSelf[0x100] = {0};
    GetModuleFileName(NULL, FilePathSelf, 0x100);

    // 1����ȡ��ǰ���ӳ����� ����
    file_size = ReadPEFile(FilePathSelf, pFileBuffer, pDosHeader, pNTHeader,
                           pSectionHeader);

    // 2������Դ�ļ�,��ȡԴ�ļ���imagebase sizeofimage����
    LPVOID pFileBufferSrc = NULL;
    DWORD dwBufferImageBaseSrc = 0;
    DWORD dwBufferSizeOfImageSrc = 0;
    GetSrcDataFromShell(pFileBuffer, pDosHeader, pNTHeader, pSectionHeader,
                        pFileBufferSrc, dwBufferImageBaseSrc,
                        dwBufferSizeOfImageSrc);
	MY_ASSERT(pFileBufferSrc);
#ifdef MY_DEBUG
	cout << hex;
	cout << "Դ�ļ�MZͷ��" <<  *((PWORD)pFileBufferSrc) << endl;
	cout << "Դ�ļ�ImageBase��" << dwBufferImageBaseSrc << endl;
	cout << "Դ�ļ�SizeOfImage��" << dwBufferSizeOfImageSrc << endl;
#endif

	// 3������PE  pImageBufferSrc
	PVOID pImageBufferSrc = NULL;
	CopyFileBufferToImageBuffer(pFileBufferSrc, &pImageBufferSrc);

    // 4���Թ���ʽ���пǳ������
	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi;
	si.cb = sizeof(si);
	::CreateProcess(FilePathSelf, NULL, NULL, NULL, NULL, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
#ifdef MY_DEBUG
	printf("error is %d\n", GetLastError());
#endif

	DWORD dwImageBaseShell = pNTHeader->OptionalHeader.ImageBase; // ��ȡ���ӳ��������imagebase

    // 5��ж����ǳ�����ļ�����
	typedef long NTSTATUS;
	typedef NTSTATUS(__stdcall * pfnZwUnmapViewOfSection)(unsigned long ProcessHandle, unsigned long BaseAddress);
	
	pfnZwUnmapViewOfSection ZwUnmapViewOfSection = NULL;
	HMODULE hModule = LoadLibrary("ntdll.dll");
	if (hModule)
	{
		ZwUnmapViewOfSection = (pfnZwUnmapViewOfSection)GetProcAddress(hModule, "ZwUnmapViewOfSection");
		if (ZwUnmapViewOfSection)
		{
			if (ZwUnmapViewOfSection((unsigned long)pi.hProcess, dwImageBaseShell))
			{ // ж�ص� ���ӳ��������ImageBase ��ַ
				printf("ZwUnmapViewOfSection success\n");
			}
		}
		FreeLibrary(hModule);
	}

    // 6����ָ����λ��(src��ImageBase)����ָ����С(src��SizeOfImage)���ڴ�(VirtualAllocEx)
	LPVOID status = ::VirtualAllocEx(pi.hProcess, (LPVOID)dwBufferImageBaseSrc, 
		dwBufferSizeOfImageSrc, 
			MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (status == NULL)
	{
		printf("error is %d\n", GetLastError());
		EXIT_ERROR("ʹ��VirtualAllocEx�����ڴ�ʱʧ��");
	}
	else
	{
#ifdef MY_DEBUG
		printf("VirtualAllocEx����ֵΪ��%x\n", status);
#endif
		//7������ɹ�����Src��PE�ļ����� ���Ƶ��ÿռ���
		if (WriteProcessMemory(pi.hProcess, (LPVOID)dwBufferImageBaseSrc, pImageBufferSrc, dwBufferSizeOfImageSrc, NULL) == 0)
		{
			printf("error: \n", GetLastError());
			EXIT_ERROR("WriteProcessMemory failure");
		}
	}

    // TODO: 8���������ռ�ʧ�ܣ������ض�λ��������λ������ռ䣬Ȼ��PE�ļ����졢���ơ��޸��ض�λ��

    // TODO: 9�������6������ռ�ʧ�ܣ����һ�û���ض�λ��ֱ�ӷ��أ�ʧ��.

	// 10���޸���ǳ����Context:
	CONTEXT cont;
	cont.ContextFlags = CONTEXT_FULL;
	::GetThreadContext(pi.hThread, &cont);
	
	DWORD dwEntryPoint = GetOep(pFileBufferSrc);	// get oep
	cont.Eax = dwEntryPoint + dwBufferImageBaseSrc; // set origin oep
	
	DWORD theOep = cont.Ebx + 8;
	DWORD dwBytes = 0;
	WriteProcessMemory(pi.hProcess, &theOep, &dwBufferImageBaseSrc, 4, &dwBytes);
	
	SetThreadContext(pi.hThread, &cont);
	//�ǵûָ��߳�
	ResumeThread(pi.hThread);
	ExitProcess(0);
	getchar();
    return 0;
}
