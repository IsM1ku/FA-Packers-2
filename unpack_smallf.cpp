#include <cstdio>
#include <string>
#include <map>
#include <iostream>
#include <filesystem>
#include <algorithm>

// return true if the file specified by the filename exists
bool file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    bool is_exist = false;
    if (fp != NULL)
    {
        is_exist = true;
        fclose(fp); // close the file
    }
    return is_exist;
}

// Utility function from https://stackoverflow.com/questions/71658440/c17-create-directories-automatically-given-a-file-path
// Returns:
//   true upon success.
//   false upon failure, and set the std::error_code & err accordingly.
bool createDirectoryRecursive(std::string const & dirName, std::error_code & err)
{
    err.clear();
    if (!std::filesystem::create_directories(dirName, err))
    {
        if (std::filesystem::exists(dirName))
        {
            // The folder already exists:
            err.clear();
            return true;    
        }
        return false;
    }
    return true;
}

std::map<int, std::string> buildFileList(FILE* file, const char* smallfName){
	std::string folderName = smallfName;
	int index = folderName.find_last_of('.');
	if(index != std::string::npos){
		folderName = folderName.substr(0, index);
	}
	std::map<int, std::string> fileList;
	while(true){
		int offset = 0; // offset in smallF that the file is located
		for(int i = 0; i <= 3; i++){
			offset += getc(file) << (8*i);
		}

		int nameLen = getc(file); // number of bytes that the filename is
		if(nameLen == 0){ // End of the file list
			std::cout << "Reached end of file list" << std::endl;
			fileList.insert({offset, ""});
			return fileList;
		}

		// Get filename
		std::string fileName;
		for(int i = 0; i < nameLen; i++){
			fileName += (char)getc(file);
		}
		std::cout << "Located file " << fileName << " at offset " << offset << std::endl;
                fileList.insert({offset, folderName + "/" + fileName});

		getc(file); // Advance to next file
	}
}

void saveFiles(FILE* file, std::map<int, std::string> fileList){
	std::map<int, std::string>::iterator fileIterator;
	int numUnpacked = 0;

	for(fileIterator = fileList.begin(); fileIterator->second != ""; fileIterator++){
		fileIterator++;
		int endOffset = fileIterator->first;
		fileIterator--;
		std::cout << "Extracting file " << fileIterator->second << " from offset range " << fileIterator->first << " to " << endOffset << std::endl;

		// Construct file path
		std::string path = fileIterator->second;
		std::replace(path.begin(), path.end(), '\\', '/');
		int index = path.find_last_of('/');
		if(index != std::string::npos){
			std::string dirpath = path.substr(0, index);
			std::error_code error;
			if(!createDirectoryRecursive(dirpath, error)){
				std::cout << "CreateDirectoryRecursive FAILED, error: " << error.message() << std::endl;
				return;
			}
		}
		// Save file
		path = "./" + path;
		if(file_exists(path.c_str())){
			std::cout << "File " << path << " already exists, overwriting." << std::endl;
		}
		FILE* fileToSave = fopen(path.c_str(), "wb");
		fseek(file, fileIterator->first, SEEK_SET);

		int size = endOffset-fileIterator->first;
		char buffer[1024*1024];
		while(size > 0){
			int numRead = fread(buffer, sizeof(char), 1, file);
			if(numRead > size){ // Past the end of the file
				fwrite(buffer, sizeof(char), size, fileToSave);
				size = 0;
			}
			else{ // Write this chunk
				fwrite(buffer, sizeof(char), numRead, fileToSave);
				size -= numRead;
			}
		}

		fclose(fileToSave);
		numUnpacked++;
	}
	std::cout << "Successfully unpacked " << numUnpacked << " files!" << std::endl;
}

int main(int argc, char* argv[]){
	std::cout << "PSEUDO Engine smallF.dat Unpacker by protorizer" << std::endl;

	const char* inputFile;
	if(argc == 1){
		inputFile = "smallf.dat";
	}
	else{
		inputFile = argv[1];
	}
	FILE* smallF = fopen(inputFile, "rb");
	if(smallF == NULL){
		std::cout << "Could not find input file " << inputFile << std::endl;
		return 0;
	}
	std::cout << "Opened file " << inputFile << std::endl;

	std::cout << "Building file list" << std::endl;
	std::map<int, std::string> files = buildFileList(smallF, inputFile);

	std::cout << "Saving files..." << std::endl;
	saveFiles(smallF, files);

	fclose(smallF);

	return 0;
}