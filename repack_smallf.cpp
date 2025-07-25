#include <iostream>
#include <filesystem>
#include <algorithm>
#include <map>

std::map<std::string, size_t> getFiles(std::string folderName){
	std::map<std::string, size_t> files;

	for (const auto & entry : std::filesystem::recursive_directory_iterator(folderName)){

		// Get file path
		std::string filePath = entry.path().string();

		// Check if its a file or just a folder
		if (filePath.find('.') != std::string::npos){

			// Determine filesize
			FILE* file = fopen(filePath.c_str(), "rb");
			fseek(file, 0, SEEK_END);
			size_t fileSize = ftell(file);
			fclose(file);

			// Trim folder name
			filePath = filePath.substr(folderName.length()+1, std::string::npos);

			std::cout << "Located file " << filePath << " of size " << fileSize << " bytes" << std::endl;
                        std::replace(filePath.begin(), filePath.end(), '\\', '/');
			files.insert({filePath, fileSize});
		}
	}
	return files;
}

size_t calcHeaderSize(std::map<std::string, size_t> fileList){
	unsigned int headerSize = 0;

	std::map<std::string, size_t>::iterator fileListIterator;
	// Each file requires 4 bytes of offset, 1 byte specifying the filename length, the filename, and one null byte
	for(fileListIterator = fileList.begin(); fileListIterator != fileList.end(); fileListIterator++){
		headerSize = headerSize + 6 + fileListIterator->first.length();
	}

	// The end of the header contains another 6 bytes
	headerSize += 6;

	return headerSize;
}

void packFile(std::string fileName, std::map<std::string, size_t> fileList, size_t headerSize){
	FILE* smallF = fopen((fileName + "_repack.dat").c_str(), "wb");

	std::map<std::string, size_t>::iterator fileListIterator;
	uint32_t currentOffset = headerSize; // Make sure its 4 bytes

	// Write headers
	for(fileListIterator = fileList.begin(); fileListIterator != fileList.end(); fileListIterator++){
		int fileNameLength = fileListIterator->first.length();

		fwrite(&currentOffset, sizeof(uint32_t), 1, smallF); // Offset
		fwrite(&fileNameLength, sizeof(char), 1, smallF); // Filename length
		fprintf(smallF, "%s", fileListIterator->first.c_str()); // Filename
		fwrite("\0", sizeof(char), 1, smallF); // Null byte

		currentOffset += fileListIterator->second; // Add filesize to the offset
	}
	// End of the header
	fwrite(&currentOffset, sizeof(uint32_t), 1, smallF); // Pointer to null block
	fwrite("\0\0", sizeof(char), 2, smallF); // Null filename and padding

	// Write files
	for(fileListIterator = fileList.begin(); fileListIterator != fileList.end(); fileListIterator++){
		// Open file
		std::string filePath = "./" + fileName + "/" + fileListIterator->first;
		std::replace(filePath.begin(), filePath.end(), '\\', '/');
		// std::cout << "Packing " << filePath << std::endl;

		// Copy contents of file to smallF
		FILE* fileToPack = fopen(filePath.c_str(), "rb");
		char buffer[1024*1024];
		while(feof(fileToPack) == 0){
			//char byte = getc(fileToPack);
			int numRead = fread(buffer, sizeof(char), sizeof(buffer), fileToPack);
			fwrite(buffer, sizeof(char), numRead, smallF);
		}
		fclose(fileToPack);
	}

	// Write null block
	for(int i = 0; i < 742; i++){
		fwrite("\0", sizeof(char), 1, smallF);
	}

	// Close the file
	fclose(smallF);

	std::cout << "Successfully packed " << fileList.size() << " files into " << fileName << "_repack.dat!" << std::endl;
}

int main(int argc, char* argv[]){
	std::cout << "PSEUDO Engine smallF.dat Repacker by protorizer" << std::endl;

	std::string folderName;
	if(argc == 1){
		folderName = "smallf";
	}
	else{
		folderName = argv[1];
	}

	if(!std::filesystem::is_directory(folderName)){
		std::cout << folderName << " is not a valid directory." << std::endl;
		return 0;
	}

	std::cout << "Building file tree..." << std::endl;
	// Recursively seek through directory and add all the filepaths to a map, with values being filesize
	std::map<std::string, size_t> files = getFiles(folderName);

	std::cout << "Successfully located " << files.size() << " files." << std::endl;

	std::cout << "Calculating file offsets..." << std::endl;
	// Calculate the size of the header that specifies the offsets of everything
	size_t headerSize = calcHeaderSize(files);

	std::cout << "Packing file..." << std::endl;
	packFile(folderName, files, headerSize); // Create the actual dat file

	return 0;
}