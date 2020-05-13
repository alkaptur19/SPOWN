#include <iostream>
#include <string>
#include "EmuFileSystem.h"

using namespace std;

int main() {
	string command;
	EmuFileSystem emufilesystem;
	cout << "To see all commands you can enter 'help'" << endl;
	while (true) {
		fflush(stdin);
		cout << " > ";
		cin >> command;
		if (command == "exit") break;
		else if (command == "help")
			emufilesystem.Help();
		else if (command == "dir")
			emufilesystem.ShowFiles();
		else if (command == "addfile")
			emufilesystem.CreateNewFile();
		else if (command == "openfile")
			emufilesystem.OpenFile();
		else if (command == "remove")
			emufilesystem.RemoveFile();
		else if (command == "reset")
			emufilesystem.ResetDisk();
		else if (command == "clear")
			system("cls");
		else cout << " Unknown command" << endl;
	}
	return 0;
}