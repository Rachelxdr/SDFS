#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <unistd.h>

using std::string;
using std::map;
using std::vector;
using std::cout;
using std::endl;
using std::ostringstream;

const static char * READ_LOCK = "/";
const static char * WRITE_LOCK = "//";
const static char * DELETE_LOCK = "///";
class Directory{
public:
	char * dir; //need to handle sigkill and remove this when node fails
    map<string,int> file_hearbeats;
    map<string,string> file_locks;
    Directory();
    void store();
	int write_file(FILE * f, char * buf, uint size);
    string get_path(string filename);
};

#endif //DIRECTORY_H
