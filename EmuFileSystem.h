#pragma once
#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <windows.h>
#include <fstream>

class EmuFileSystem {
public: 
		static const int MAX_FILE_NAME_LENGTH_ = 20; 
		static const int AMOUNT_OF_BLOCKS_ = 1000; 
		static const int MAX_AMOUNT_OF_FILES_ = 100; 
		static const int BLOCK_DATA_SIZE = 127; 
		static const char SOURCE_NAME_[];
		typedef char byte;

		// ��������� �������� � ������ � ������������ � �������� ������������.
		// ����� �������� ������������ ����������� � 1, ����� �������, ����� struture �����
		// ����������� �� ������ ����� (��������� ����� ����� ��������� ������)
#pragma pack(push, 1) //������������ �������� �� 1 �����
		struct FileMetaDataTableField
		{
			char name[MAX_FILE_NAME_LENGTH_] = { 0 };
			int begining_offset = 0;
		};
#pragma pack(pop)

	typedef FileMetaDataTableField* FileMetaDataTable; //����� "���" ��������� �� ��������

#pragma pack(push, 1)
		struct Block //127 + 1 + 4 = 132 - ������ �����
		{
			byte busy_flag = 0;
			char data[BLOCK_DATA_SIZE] = { 0 };
			int next_offset = 0;
		};
#pragma pack(pop)
public:
		static const int FIRST_BLOCK_OFFSET_ = MAX_AMOUNT_OF_FILES_ * sizeof(FileMetaDataTableField) + 1;//���������� �������� ������� �����
		static const int FMDT_SIZE = FIRST_BLOCK_OFFSET_ - 1; //������ ����� ����������
		void Help();
		void ShowFiles();
		void CreateNewFile();
		void RemoveFile();
		void ResetDisk();
		void OpenFile();
		EmuFileSystem();
		~EmuFileSystem();

private:
		void SaveFMDT();
		void SaveBlock(Block block, int offset);
		Block ReadBlock(int offset);
		void ClearBlock(Block &block);
		int FindFreeBlock(bool next);
		void CopyString(char* destinantion, int destination_max_size, const char* source);
		std::string FileToBuffer(int begining_offset);
		std::string ProcessFileBuffer(std::string);
		void SaveFile(std::string, int begining_offset);

		std::fstream source_file_;
		//File MetaData Table == fmdt
		FileMetaDataTableField fmdt_[MAX_AMOUNT_OF_FILES_];
		int free_space_;
	};
#endif




