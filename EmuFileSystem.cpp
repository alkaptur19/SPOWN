#include "EmuFileSystem.h"
#include <conio.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;


const char EmuFileSystem::SOURCE_NAME_[] = "phys_memory.bin";

EmuFileSystem::EmuFileSystem() {
	this->free_space_ = sizeof(Block) * AMOUNT_OF_BLOCKS_; //кол-во свободного места для файлов
	SecureZeroMemory(this->fmdt_, this->MAX_AMOUNT_OF_FILES_ * sizeof(FileMetaDataTableField)); //обнуляем память
	this->source_file_.open(this->SOURCE_NAME_, ios::binary | ios::in | ios::out | ios::ate); //создаем исходный файл, который представляет нашу файловую систему, установив указатель на конец файла
	//если файл был уже создан, то считываем его метаданные, если нет, то создаем новый
	if (!this->source_file_.is_open()) { //если файл не открылся
		this->source_file_.open(this->SOURCE_NAME_, ios::binary | ios::out | ios::ate); //открываем его еще раз для операции записи в поток
		
		const int empty_file_size = this->AMOUNT_OF_BLOCKS_ * sizeof(Block) + this->FMDT_SIZE; //размер пустого файла
		byte set_arry[empty_file_size]; //char массив размером с пустой файл
		SecureZeroMemory(set_arry, empty_file_size);
		this->source_file_.write(set_arry, empty_file_size); //записываем в открытый файл массив из 0

		this->source_file_.close(); //закрываем файл
		this->source_file_.open(this->SOURCE_NAME_, ios::binary | ios::in | ios::out | ios::ate); //опять открываем файл для чтения/записи
	}
	else {
		this->source_file_.seekg(0); //позиция в начало файла
		this->source_file_.read((byte*)this->fmdt_, sizeof(FileMetaDataTableField) * this->MAX_AMOUNT_OF_FILES_); //считать метаданные этого файла
	}
}

EmuFileSystem::~EmuFileSystem() {
	this->source_file_.close();
}

void EmuFileSystem::Help()
{
	cout << endl;
	cout << "Available commands" << endl;
	cout << "----------------------------------------" << endl;
	cout << "dir - show all files" << endl;
	cout << "addfile - adding a new file" << endl;
	cout << "openfile - open file" << endl;
	cout << "remove - removing a component" << endl;
	cout << "reset - reset disk" << endl;
	cout << "clear - clear screen" << endl;
	cout << "exit - exit the programm" << endl;
	cout << "----------------------------------------" << endl;
	cout << endl;
}

void EmuFileSystem::ResetDisk()
{
	this->free_space_ = sizeof(Block) * AMOUNT_OF_BLOCKS_;  //возвращем все свобоную память
	SecureZeroMemory(this->fmdt_, this->MAX_AMOUNT_OF_FILES_ * sizeof(FileMetaDataTableField)); //обнуляем структуры метаданных

	this->source_file_.close();
	this->source_file_.open(this->SOURCE_NAME_, ios::binary | ios::out | ios::trunc); //открываем исходный файл для записис с обнуление данных

	const int empty_file_size = this->AMOUNT_OF_BLOCKS_ * sizeof(Block) + this->FMDT_SIZE;
	byte set_arry[empty_file_size];
	SecureZeroMemory(set_arry, empty_file_size);
	this->source_file_.write(set_arry, empty_file_size); //записываем туда 0
	
	this->source_file_.close(); 
	this->source_file_.open(this->SOURCE_NAME_, ios::binary | ios::in | ios::out | ios::ate); //открываем для чтения/записи, установив указатель на конец файла

	cout << endl << endl << "The disk was successfully reseted..." << endl << endl;
}

void EmuFileSystem::ShowFiles() {
	cout << endl << "All files: " << endl;
	for (int i = 0; i < this->MAX_AMOUNT_OF_FILES_; i++) {
		if (this->fmdt_[i].begining_offset != 0) { //если смещение файла не равно 0, т.е. он существует, выводим его имя не понятно
			cout << "-> " << this->fmdt_[i].name << endl;
	    }
	}
	cout << endl;
}

void EmuFileSystem::CreateNewFile() {
	cout << "Please, enter the name of a new file:" << endl;
	string new_file_name;
	cin >> new_file_name;
	for (int i = 0; i < this->MAX_AMOUNT_OF_FILES_; i++) {
		if (strcmp(this->fmdt_[i].name, new_file_name.c_str()) == 0) //проверяем, нет ли с таким именем уже созданных файлов
		{
			cout << endl << "File name is already in use, please repeat..." << endl;
			return;
		}
	}

	for (int i = 0; i < this->MAX_AMOUNT_OF_FILES_; i++) {
		if (this->fmdt_[i].begining_offset == 0) { //ищем свободную структура, не занятую метаданными другого файла
			this->fmdt_[i].begining_offset = this->FindFreeBlock(false); //возвращается смещение свободного блока
			if (this->fmdt_[i].begining_offset == 0) //если вернулся ноль, то не нашли блок
				break;

			int copy_size = this->MAX_FILE_NAME_LENGTH_ < new_file_name.length() ? this->MAX_FILE_NAME_LENGTH_ : new_file_name.length();// если имя нового файла больше, чем максимальная длина, то записывается максимальная, иначе длина введенного
			CopyString(this->fmdt_[i].name, copy_size, new_file_name.c_str()); //копируем в стркутуру метаданных имя новго файла без 0 на конце

			this->SaveFMDT();//переписываем все метаданные с учетом новых

			Block temp_block;
			temp_block.busy_flag = 1; //устанавливаем флажок, что этот блок занят
			this->SaveBlock(temp_block, this->fmdt_[i].begining_offset); //записываем блок по найденному смещению

			return;
		}
	}
	cout << endl << endl << "Max amount of files is reached! Free some space, to perform this operation..." << endl << endl;
}

void EmuFileSystem::SaveFMDT()
{
	this->source_file_.seekp(0); //в начальную позцицию(доступ к потоку вывода)
	this->source_file_.write((byte*)this->fmdt_, sizeof(FileMetaDataTableField) * this->MAX_AMOUNT_OF_FILES_); //переписываем все метаданные с начала
	this->source_file_.flush(); //сброс буферов
}

void EmuFileSystem::SaveBlock(Block block, int offset)
{
	this->source_file_.seekp(offset); //на начало смещения начала смобоного блока
	this->source_file_.write((byte*)&block, sizeof(Block)); //записываем туда блок, пока что только пустой и с флагом
	this->source_file_.clear(); //переводим поток номрмальное состояние(если были ошибкии
	this->source_file_.flush(); //сбрасываем буфер файла
}

void EmuFileSystem::ClearBlock(Block &block) {
	block.next_offset = 0; 
	block.busy_flag = 0; 
	SecureZeroMemory(block.data, this->BLOCK_DATA_SIZE); //обнуляем память
}

EmuFileSystem::Block EmuFileSystem::ReadBlock(int offset) {
	Block temp_block;
	this->source_file_.seekg(offset); //на начало блока
	this->source_file_.read((byte*)&temp_block, sizeof(Block));  //считываем блок 
	this->source_file_.clear();
	return temp_block;
}

int EmuFileSystem::FindFreeBlock(bool next) {
	byte current_flag = 0;
	int offset = this->FIRST_BLOCK_OFFSET_; //смещение первого блока памяти
	for (int i = 0; i < this->AMOUNT_OF_BLOCKS_; i++) {
		this->source_file_.seekg(offset); //смещаемся на +=132 байт каждый раз
		source_file_.read(&current_flag, sizeof(byte)); //считываем первую переменную блока, отвечающую за то, свободен он или занят

		if (current_flag == 0) { //если значение 0 - то он свободен
			if (next)
			{
				next = false;
				continue;
			}
			return offset; //получаем начало смещения свободного блока
		}
		offset += sizeof(Block); //каждый проход по циклу добавляем 132 байт
	}
	return 0; //если не нашли возвращаем 0
}

void EmuFileSystem::CopyString(char* destinantion, int destination_max_size, const char* source) {//куда, сколько, откуда
	for (int i = 0; i < destination_max_size && source[i] != '\0'; i++)	{
		destinantion[i] = source[i];
	}
}

void EmuFileSystem::RemoveFile()
{
	cout << "Please, enter the name of a file to delete" << endl;

	cin.clear();
	string deleting_file;
	cin >> deleting_file;

	for (int i = 0; i < this->MAX_AMOUNT_OF_FILES_; i++) {
		if (strcmp(this->fmdt_[i].name, deleting_file.c_str()) == 0) {//ищем в метаданных файл с атким названием+
			int offset = this->fmdt_[i].begining_offset; //получаем его смещение в физ памяти
			while (true) {
				Block temp_block = this->ReadBlock(offset); //получаем блок, который привязан к этому файлу по имени

				int next_offset = temp_block.next_offset;  //получаем смещение следующего блока, если файл не влез в 1
				
				this->ClearBlock(temp_block); //очищаем этот блок

				this->SaveBlock(temp_block, offset); //сохраняем сброшенный блок

				if (next_offset == 0)
					break;
				else
					offset = next_offset; //устанавливаем сещение на следующий блок, относящийся к файлу
			}
			this->fmdt_[i].begining_offset = 0; //изменяем блок метаданных, отвечающих за этот файл
			SecureZeroMemory(this->fmdt_[i].name, this->MAX_FILE_NAME_LENGTH_);

			this->SaveFMDT(); //сохраняем иземеннную строку метаданных

			return;
		}
	}
	cout << endl << "Incorrect file name, please repeat..." << endl;
}

string EmuFileSystem::FileToBuffer(int begining_offset) {
	string result_str;
	int offset = begining_offset;
	while (true) {
		Block temp_block = this->ReadBlock(offset); //читаем блок по смещению файла

		char temp_buffer[this->BLOCK_DATA_SIZE + 1] = { 0 };
		this->CopyString(temp_buffer, this->BLOCK_DATA_SIZE, temp_block.data); //купируем во временный буфер данные из блока файла

		result_str.append(temp_buffer); //заносим в стринговую переменную наш временный буфер

		if (temp_block.next_offset == 0) //если нету следующего блока, закрепленного за файлом - возвращаем буфер файла
			return result_str;
		else
			offset = temp_block.next_offset; //продолжаем считывать буфер
	}
}

void EmuFileSystem::OpenFile() {
	cout << "Please, enter the name of a file" << endl;

	string current_file;
	cin >> current_file;

	for (int i = 0; i < this->MAX_AMOUNT_OF_FILES_; i++) {
		if (strcmp(this->fmdt_[i].name, current_file.c_str()) == 0) { //ищем совпадение имени файла
			string fileBuffer = this->FileToBuffer(this->fmdt_[i].begining_offset);  //считали буфер файла со всех блоков

			string processed;
			processed = this->ProcessFileBuffer(fileBuffer); //получили измененную строку пользователем(он же файловый буфер)

			// если файл очищен, просто очистите все блоки, связанные с ним
			// а затем восстановить первый (но оставить его пустым), оставив при этом пустой файл, где мы все еще можем что-то написать.
			if (processed.length() == 0) {
				int offset = this->fmdt_[i].begining_offset; 
				while (true) {
					Block temp_block = this->ReadBlock(offset); 
					int next_offset = temp_block.next_offset;
					this->ClearBlock(temp_block); //очищаем блок
					this->SaveBlock(temp_block, offset); //сохраняем очищенный блок

					if (next_offset == 0)
						break;
					else
						offset = next_offset; //и следующий, если есть
				}

				Block firstFileBlock; //создаем новый пустой но с флагом "занят"
				firstFileBlock.busy_flag = 1; 
				this->SaveBlock(firstFileBlock, this->fmdt_[i].begining_offset); //сохраняем новый блок

				return;
			}
			//если пользователь оставил буфер не пустым
			this->SaveFile(processed, this->fmdt_[i].begining_offset);
			return;
		}
	}

	cout << endl << "Incorrect file name, please repeat..." << endl;
	return;
}

void EmuFileSystem::SaveFile(string processed, int begining_offset) {
	int unprocessed_length = processed.length(); //длина строки
	int offset = begining_offset;
	while (true) {
		int min_size = unprocessed_length < this->BLOCK_DATA_SIZE ? unprocessed_length : this->BLOCK_DATA_SIZE;//если длина строки меньше размера блока то минимальный размер - размер строки, иначе размер блока

		Block temp_block = this->ReadBlock(offset); //первый блок файла
		SecureZeroMemory(temp_block.data, this->BLOCK_DATA_SIZE); //обнуляем
		temp_block.busy_flag = 1;

		this->CopyString(temp_block.data, min_size, processed.c_str()); //копируем екобходимое кол-во символов

		this->SaveBlock(temp_block, offset); //сохрняем блок

		unprocessed_length -= min_size; // отнимаем записанный
		processed.erase(0, min_size); //удаляем с 0 кол-во записанных данных

		// если весь файловый буфер был обработан
		if (unprocessed_length == 0) {
			// если остались неиспользованные блоки, очистите их
			if (temp_block.next_offset != 0) {
				int clear_offset = temp_block.next_offset;
				temp_block.next_offset = 0; //убираем указатель на след блок
				this->SaveBlock(temp_block, offset);
				while (true) {
					Block clear_block = this->ReadBlock(clear_offset); //считываем смещенный блок

					int old_offset = clear_offset;
					clear_offset = clear_block.next_offset; //значение следующего

					this->ClearBlock(clear_block); //очищаем этот блок
					this->SaveBlock(clear_block, old_offset); //сохраняем

					if (clear_offset == 0) // пока не закончатся блоки
						break;
				}
			}
			else return; // если был только 1 блок, то завершаем изменение блоков
		}

		// если блок был последним из заполненных блоков
		if (temp_block.next_offset == 0) {
			// добавить новые блоки, если не весь файловый буфер был обработан
			if (unprocessed_length != 0) {
				temp_block.next_offset = this->FindFreeBlock(false); //ищем новый блок и заносим его смещение
				// если свободных блоков больше нет
				if (temp_block.next_offset == 0) { // не нашли - значит нету места
					cout << endl << endl << "Not enough space on disk! Some information was lost!" << endl << endl;
					return;
				}
				this->SaveBlock(temp_block, offset); //сохраняем блок
			}
			else {
				return;
			}
		}
		offset = temp_block.next_offset; // если есть еще подключенные блоки к файлу, то переходим к ним
	}
}

string EmuFileSystem::ProcessFileBuffer(string fileBuffer) {
	string processed = fileBuffer; //копируем наш буфер файла
	while (true) {
		system("cls");
		cout << processed << endl << endl;
		cout << "Choose an option:" << endl;
		cout << "1. Add to position" << endl;
		cout << "2. Remove from position" << endl;
		cout << "3. Clear file" << endl;
		cout << "0. Close file and save changes" << endl;

		switch (_getch()) {
		case '0':
		{
			return processed; //закончили операции со строкой
		}
		case '1':
		{
			cout << "Please, select option:" << endl;
			cout << "1. Append to end" << endl;
			cout << "2. Append from position" << endl;

			char symbol = 0;
			while (symbol != '1' && symbol != '2') {
				symbol = _getch();
			}
			string additional;

			cout << "Continue..." << endl;
			cin.ignore();
			cin.clear();
			cout << "Please, enter the text:" << endl;
			getline(cin, additional);

			if (symbol == '1') {
				processed.append(additional);
			}
			else {
				int adding_position = -1;
				while (adding_position < 0 || (adding_position > processed.length() - 1)) { //проверяем, пока не будет введена существующая позиция
					cout << "Please, enter the position, where to start adding text..." << endl;
					cin >> adding_position;
				}
				processed.insert(adding_position, additional); //вставляем в буфер файла новую строку с введенной позиции
			}
			break;
		}
		case '2':
		{
			int delete_position = -1;
			int delete_size = -1;

			while (delete_position < 0 || delete_size < 0 || ((delete_size + delete_position) > processed.length() + 1)) {
				cout << "Please, enter the position, where to start deleting..." << endl;
				cin >> delete_position;
				cout << "Please, enter the size of a fragment..." << endl;
				cin >> delete_size;
			}
			processed.erase(delete_position, delete_size); //удаляем с введенной позиции
			break;
		}
		case '3':
		{
			processed.clear(); //очищаем строку
			break;
		}
		}
	}
}
