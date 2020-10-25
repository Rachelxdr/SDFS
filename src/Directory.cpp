#include "../inc/Directory.h"

Directory::Directory(){
    dir = mkdtemp((char*)"XXXXXX");
}

void Directory::store(){
    cout << "[STORE] " << endl;
    for(auto& element: file_hearbeats){
        cout << "FILE - " << element.first.c_str() << endl;
    }
}

int Directory::write_file(FILE * f, char * buf, uint size){
	size_t written = 0;
	while (written < size){
		int actuallyWritten = fwrite(buf + written, 1, size - written, f);
		if (actuallyWritten <= 0){
			fprintf(stderr, "coudln't write to local file\n");
        	return -1;
		}
		else written += actuallyWritten;
	}
    return 0;
}

string Directory::get_path(string filename){
    ostringstream stringStream;
    stringStream << dir << "/" << filename;
    return stringStream.str();
}
