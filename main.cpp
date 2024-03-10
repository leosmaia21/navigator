#include <filesystem>
#include <fstream>
#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;


#define FILE 1
#define FOLDER 2
#define OTHER 3


#define TYPE_FILE 4
#define TYPE_FOLDER 5
#define TYPE_EXEC 6
#define SELECTED_FILE 7
#define SELECTED_FOLDER 8
#define SELECTED_EXEC 9

#define MAXSIZEPARENT 35

struct file {
	string fullName;
	string name;
	int type;
	fs::perms perm;
	uintmax_t size;
};

string permToString(fs::perms p) {
	string result;
	result += ((p & fs::perms::owner_read) != fs::perms::none) ? 'r' : '-';
	result += ((p & fs::perms::owner_write) != fs::perms::none) ? 'w' : '-';
	result += ((p & fs::perms::owner_exec) != fs::perms::none) ? 'x' : '-';
	result += ((p & fs::perms::group_read) != fs::perms::none) ? 'r' : '-';
	result += ((p & fs::perms::group_write) != fs::perms::none) ? 'w' : '-';
	result += ((p & fs::perms::group_exec) != fs::perms::none) ? 'x' : '-';
	result += ((p & fs::perms::others_read) != fs::perms::none) ? 'r' : '-';
	result += ((p & fs::perms::others_write) != fs::perms::none) ? 'w' : '-';
	result += ((p & fs::perms::others_exec) != fs::perms::none) ? 'x' : '-';
	return result;
}

string getTheLast(string path){
	int last = path.find_last_of('/');
	string file = path.substr(last + 1, path.length()-1);
	string ret;
	if (last != 0){
		string file = path.substr(last + 1, path.length()-1);
		if (file.length() <= MAXSIZEPARENT){
			ret = file;
		}else
			ret = file.substr(0, MAXSIZEPARENT);
	}else
		ret = "/";
	return  ret;
}

void getFolderFiles(vector<file> &files, string directory) {
	files.clear();
	for (const auto &entry : fs::directory_iterator(directory)) {
		if (fs::is_regular_file(entry.path())) {
			files.emplace_back(file{entry.path().string(), getTheLast(entry.path().string()), FILE, fs::status(entry.path()).permissions(), fs::file_size(entry.path())});
		} else if (fs::is_directory(entry.path())) {
			files.emplace_back(file{entry.path().string(), getTheLast(entry.path().string()), FOLDER, fs::status(entry.path()).permissions(), 0});
		} else {
			files.emplace_back(file{entry.path().string(), getTheLast(entry.path().string()), OTHER, fs::status(entry.path()).permissions(), 0});
		}
	}
}

void printContent(file &content, int selected){
	if (!selected){
		if (content.type == FOLDER){
			attron(COLOR_PAIR(TYPE_FOLDER));
			printw("| %s ", content.name.c_str());
			attroff(COLOR_PAIR(TYPE_FOLDER));
		} else if ((content.perm & fs::perms::owner_exec) != fs::perms::none){
			attron(COLOR_PAIR(TYPE_EXEC));
			printw("| %s ", content.name.c_str());
			attroff(COLOR_PAIR(TYPE_EXEC));
		} else if (content.type == FILE){
			attron(COLOR_PAIR(TYPE_FILE));
			printw("| %s ", content.name.c_str());
			attroff(COLOR_PAIR(TYPE_FILE));
		}
	}else {
		if (content.type == FOLDER){
			attron(COLOR_PAIR(SELECTED_FOLDER));
			printw("| %s ", content.name.c_str());
			attroff(COLOR_PAIR(SELECTED_FOLDER));
		} else if ((content.perm & fs::perms::owner_exec) != fs::perms::none){
			attron(COLOR_PAIR(SELECTED_EXEC));
			printw("| %s ", content.name.c_str());
			attroff(COLOR_PAIR(SELECTED_EXEC));
		} else if (content.type == FILE){
			attron(COLOR_PAIR(SELECTED_FILE));
			printw("| %s ", content.name.c_str());
			attroff(COLOR_PAIR(SELECTED_FILE));
		}
	}
}

int rangestart = 0;
int rangeend = 0;
void print(vector<file> &files, vector<file> &parentFiles, int &cursor, int rows){
	string line;
	int parentFilesSize = parentFiles.size();
	int filesSize = files.size();

	if (cursor < rangestart){
		rangestart--;
		rangeend--;
	}else if (cursor > rangeend){
		rangestart++;
		rangeend++;
	}

	int indexParent = 0;
	for (int index = rangestart; index <= rangeend; index++){
		if (indexParent < parentFilesSize){
			if (parentFiles[indexParent].name.length() < MAXSIZEPARENT)
				parentFiles[indexParent].name += string(MAXSIZEPARENT - parentFiles[index].name.length(), ' ');
			if (fs::current_path().string() == parentFiles[indexParent].fullName)
				printContent(parentFiles[indexParent], 1);
			else
				printContent(parentFiles[indexParent], 0);
		} else
			printw("%s", string(MAXSIZEPARENT + 3, ' ').c_str());
		indexParent++;

		if (index < filesSize)
			printContent(files[index], index==cursor);
		printw("\n");

	}
}

int main() {
	initscr(); // Initialize ncurses
	raw();     // Disable line buffering
	keypad(stdscr, TRUE); // Enable special keys

    start_color();

    // Define color pairs
    init_pair(TYPE_FILE, COLOR_WHITE, COLOR_BLACK); //folder
    init_pair(TYPE_FOLDER, COLOR_BLUE, COLOR_BLACK); //folder
    init_pair(TYPE_EXEC, COLOR_GREEN, COLOR_BLACK);  //otherfile
    init_pair(SELECTED_FILE, COLOR_BLACK, COLOR_WHITE); //folder
    init_pair(SELECTED_FOLDER, COLOR_BLACK, COLOR_BLUE); //folder
    init_pair(SELECTED_EXEC, COLOR_BLACK, COLOR_GREEN);  //otherfile
	noecho();
	curs_set(0);

	vector<file> files(40);
	vector<file> parentFiles(40);

	int cursor = 0;
	int rows, cols;
	getmaxyx(stdscr, rows, cols);
	rows--;

	rangeend = rows;

	getFolderFiles(files, fs::current_path().string());
	getFolderFiles(parentFiles, fs::current_path().parent_path().string());

	clear(); // Clear the screen
	print(files, parentFiles, cursor, cols);
	int ch;
	while ((ch = getch()) != 'q') {
		if (ch == KEY_RESIZE)
			continue;
		if (ch == 'j' && cursor < files.size() - 1)
			cursor++;
		else if (ch == 'k' && cursor > 0)
			cursor--;
		else if (ch == KEY_ENTER or ch == '\n' or ch == 'l'){
			if (fs::is_directory(files[cursor].fullName)){
				fs::current_path(files[cursor].fullName);
				getFolderFiles(files, fs::current_path().string());
				getFolderFiles(parentFiles, fs::current_path().parent_path().string());
				cursor = 0;
			}
		} else if (ch == KEY_BACKSPACE or ch == '\b' or ch == 'h' or ch == 127){
			if (fs::current_path().parent_path().string() != "/"){
				string oldPath = fs::current_path().string();
				fs::current_path(fs::current_path().parent_path().string());
				getFolderFiles(files, fs::current_path().string());
				getFolderFiles(parentFiles, fs::current_path().parent_path().string());
				int index = 0;
				for (int index = 0; index < files.size(); index++){
					if (files[index].fullName == oldPath)
						break;
				cursor = index+1;
				}
			}
		}
		clear(); // Clear the screen
		print(files, parentFiles, cursor, rows);
	}

	ofstream outputFile("/tmp/.directorytmp", ios::trunc);
	if (outputFile.is_open()) {
		outputFile << fs::current_path().string();
		outputFile.close();
	}
	endwin(); // Clean up ncurses

	return 0;
}
