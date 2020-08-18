#include "StdAfx.h"
#include "head.h"

int main()
{
	LPVOID pFileBuffer = NULL;
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS32 pNTHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;

	LPVOID pFileBuffer_src = NULL;
	PIMAGE_DOS_HEADER pDosHeader_src = NULL;
	PIMAGE_NT_HEADERS32 pNTHeader_src = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader_src = NULL;

	char file_path_shell[256];
	char file_path_src[256];
	memset(file_path_shell, 0, 256);
	memset(file_path_src, 0, 256);

	printf("������ǳ����λ�ã�");
	scanf("%s", file_path_shell);
	printf("������Դ�����λ�ã�");
	scanf("%s", file_path_src);

	DWORD file_size = ReadPEFile(file_path_shell, pFileBuffer, pDosHeader,
		pNTHeader, pSectionHeader);

	// ��ȡԴ�ļ�
	DWORD file_size_src = ReadPEFile(file_path_src, pFileBuffer_src, pDosHeader_src,
		pNTHeader_src, pSectionHeader_src);

	// ������
	LPVOID pNewFileBuffer = NULL;
	AddNewSec(&pNewFileBuffer, pFileBuffer, pDosHeader, pNTHeader, pSectionHeader, file_size, file_size_src);
	file_size += file_size_src; // �����ļ���С
	pFileBuffer = pNewFileBuffer;
	// ���Ƶ�����
	memcpy(
		(LPVOID)((PBYTE)pFileBuffer + file_size - file_size_src),
		pFileBuffer_src,
		file_size_src
		);

	char file_path_out[256];
	memset(file_path_out, 0, 256);
	printf("�ӿ�����ɣ�������Ҫ�����λ�ã�");
	scanf("%s", file_path_out);
	FILE* fp_out;
	fp_out = fopen(file_path_out, "wb");
	MY_ASSERT(fp_out);
	fwrite(pFileBuffer, file_size, 1, fp_out);
	fclose(fp_out);
}

void AddNewSec(OUT LPVOID* pNewFileBuffer, IN LPVOID pFileBuffer, PIMAGE_DOS_HEADER pDosHeader,
	PIMAGE_NT_HEADERS32 pNTHeader,
	PIMAGE_SECTION_HEADER pSectionHeader, DWORD file_size, DWORD dwAddSize)
{
	PIMAGE_FILE_HEADER pImageFileHeader =
		(PIMAGE_FILE_HEADER)((PBYTE)pDosHeader + pDosHeader->e_lfanew + 4);

	PIMAGE_OPTIONAL_HEADER32 pImageOptionalHeader = (PIMAGE_OPTIONAL_HEADER32)(
		(PBYTE)pImageFileHeader + sizeof(IMAGE_FILE_HEADER));

	if (((PBYTE)pImageOptionalHeader->SizeOfHeaders -
		((PBYTE)pDosHeader->e_lfanew + IMAGE_SIZEOF_FILE_HEADER +
			pImageFileHeader->SizeOfOptionalHeader +
			40 * pImageFileHeader->NumberOfSections)) < 80)
		EXIT_ERROR("�ռ䲻�㣬�����ڱ�ʧ�ܣ�");

	DWORD numOfSec = pImageFileHeader->NumberOfSections;
	// 1) ���һ���µĽڱ�(����copyһ��)
	memcpy(
		(LPVOID)(pSectionHeader + numOfSec), // ��Ҫ������λ��
		(LPVOID)(pSectionHeader), // ��һ����
		sizeof(IMAGE_SECTION_HEADER)
	); // ����һ���ڣ�����ڣ�����

	// 2) �������ں��� ���һ���ڴ�С��000
	memset((LPVOID)(pSectionHeader + numOfSec + 1), 0, sizeof(IMAGE_SECTION_HEADER));


	// 3) �޸�PEͷ�нڵ�����
	pImageFileHeader->NumberOfSections += 1;

	// 4) �޸�sizeOfImage�Ĵ�С
	pImageOptionalHeader->SizeOfImage += 0x1000;

	// 5�����������ڱ������
	// �޸ĸýڱ������
	memcpy((LPVOID)(pSectionHeader + numOfSec), g_NameOfNewSectionHeader, sizeof(g_NameOfNewSectionHeader));
	// �޸ĸýڱ���������Ҫ����
	(pSectionHeader + numOfSec)->Misc.VirtualSize = 0x1000; // ����ǰ�Ĵ�С����Ϊ1000����
	// ����ǰһ���ڱ�������޸�virtualAddress
	DWORD t_Add = 0; // (pSectionHeader + numOfSec - 1)->VirtualAddress ���� t_Add ΪVirtualAddress

	if ((pSectionHeader + numOfSec - 1)->Misc.VirtualSize < (pSectionHeader + numOfSec - 1)->SizeOfRawData)
	{ // ���VirtualSizeС��SizeOfRawData
		t_Add = (pSectionHeader + numOfSec - 1)->SizeOfRawData;
	}
	else
	{
		if (((pSectionHeader + numOfSec - 1)->Misc.VirtualSize % 0x1000) == 0) // ������ܱ�0x1000����
			t_Add = (pSectionHeader + numOfSec - 1)->Misc.VirtualSize;
		else
			t_Add = ((((pSectionHeader + numOfSec - 1)->Misc.VirtualSize / 0x1000) + 1)) * 0x1000;
	}
	MY_ASSERT(t_Add);
	(pSectionHeader + numOfSec)->VirtualAddress = (pSectionHeader + numOfSec - 1)->VirtualAddress
		+ t_Add;
	// �޸�sizeofRawData
	(pSectionHeader + numOfSec)->SizeOfRawData = 0x1000;
	// ����PointerToRawData
	(pSectionHeader + numOfSec)->PointerToRawData = (pSectionHeader + numOfSec - 1)->PointerToRawData +
		(pSectionHeader + numOfSec - 1)->SizeOfRawData;
	// (pSectionHeader + numOfSec)->Characteristics = (pSectionHeader->Characteristics | (pSectionHeader + numOfSec - 1)->Characteristics);

	// 6) ��ԭ�����ݵ��������һ���ڵ�����(�ڴ�����������) (���Ƶ��µ�LPVOID��ȥ)
	*pNewFileBuffer = malloc(file_size + dwAddSize);
	memcpy(*pNewFileBuffer, pFileBuffer, file_size);
}

DWORD ReadPEFile(IN LPSTR file_in, OUT LPVOID& pFileBuffer,
	PIMAGE_DOS_HEADER& pDosHeader, PIMAGE_NT_HEADERS32& pNTHeader,
	PIMAGE_SECTION_HEADER& pSectionHeader)
{
	FILE* fp;
	fp = fopen(file_in, "rb");
	if (!fp)
		EXIT_ERROR("fp == NULL!");
	DWORD file_size = 0;
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	LPVOID t = malloc(file_size);
	if ((fread(t, file_size, 1, fp) != 1) || t == NULL)
		EXIT_ERROR("fread error or malloc error!");

	pFileBuffer = t;
	MY_ASSERT(pFileBuffer);

	pDosHeader = (PIMAGE_DOS_HEADER)(pFileBuffer);
	MY_ASSERT(pDosHeader);
	MY_ASSERT((pDosHeader->e_magic == IMAGE_DOS_SIGNATURE));

	pNTHeader =
		(PIMAGE_NT_HEADERS32)((PBYTE)pFileBuffer + pDosHeader->e_lfanew);
	if (pNTHeader->FileHeader.SizeOfOptionalHeader != 0xe0)
		EXIT_ERROR("this is not a 32-bit executable file.");

	pSectionHeader = (PIMAGE_SECTION_HEADER)(
		(PBYTE)pNTHeader + sizeof(IMAGE_NT_SIGNATURE) +
		sizeof(IMAGE_FILE_HEADER) + pNTHeader->FileHeader.SizeOfOptionalHeader);
	fclose(fp);
	return file_size;
}

DWORD RVA_TO_FOA(LPVOID pFileBuffer, PIMAGE_DOS_HEADER pDosHeader,
	PIMAGE_NT_HEADERS32 pNTHeader,
	PIMAGE_SECTION_HEADER pSectionHeader, IN DWORD RVA)
{
	if (RVA < pNTHeader->OptionalHeader.SizeOfHeaders)
		return RVA;

	for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
		if (RVA >= pSectionHeader[i].VirtualAddress &&
			RVA < pSectionHeader[i].VirtualAddress +
			pSectionHeader[i].Misc.VirtualSize)
		{
			return (RVA - pSectionHeader[i].VirtualAddress +
				pSectionHeader[i].PointerToRawData);
		}
	}

	EXIT_ERROR("rva to foa failure!");
}

DWORD FOA_TO_RVA(LPVOID pFileBuffer, PIMAGE_DOS_HEADER pDosHeader,
	PIMAGE_NT_HEADERS32 pNTHeader,
	PIMAGE_SECTION_HEADER pSectionHeader, IN DWORD FOA)
{
	if (FOA < pNTHeader->OptionalHeader.SizeOfHeaders)
		return FOA;

	for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++)
	{
		if (FOA >= pSectionHeader[i].PointerToRawData &&
			FOA < pSectionHeader[i].PointerToRawData +
			pSectionHeader[i].Misc.VirtualSize)
		{
			return (FOA - pSectionHeader[i].PointerToRawData +
				pSectionHeader[i].VirtualAddress);
		}
	}

	EXIT_ERROR("foa to rva error!");
}