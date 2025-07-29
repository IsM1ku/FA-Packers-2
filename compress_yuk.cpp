#include <iostream>
#include <filesystem>
#include <string>
#include <cstdio>

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

std::vector<std::string> readFiles(std::string inputDir){
	std::vector<std::string> files;
	size_t fileSize = 0;
	for(int i = 0; i < 8; i++){
		std::string filePath = inputDir + "/" + inputDir + "_Stream" + std::to_string(i) + ".wav";
		if(!file_exists(filePath.c_str())){
			std::cout << "Error: " << filePath << " not found. Make sure you have 8 .wav files following the proper naming conventions." << std::endl;
			return files;
		}
		FILE* file = fopen(filePath.c_str(), "rb");
		fseek(file, 24, SEEK_SET);
		int bitrate;
		fread(&bitrate, sizeof(int), 1, file);
		if(bitrate != 48000){
			std::cout << "Error: The bitrate of " << filePath << " is " << bitrate << ". The input file must have a bitrate of 48000." << std::endl;
			return files;
		}
		fseek(file, 0, SEEK_END);
		size_t tmp = ftell(file);
		if(fileSize != 0 && fileSize != tmp){
			std::cout << "ERROR: Files are not the same size. Please ensure all 8 .wav files are exactly the same length." << std::endl;
			return files;
		}
		fileSize = tmp;
		files.push_back(filePath);
		fclose(file);
	}

	return files;
}

std::vector<FILE*> convertFiles(std::vector<std::string> files){
	std::vector<FILE*> atracFiles;

	for(int i = 0; i < files.size(); i++){
		std::string tmpPath = files[i] + "_" + std::to_string(i);
		std::string command = ".\\at3tool\\ps3_at3tool.exe -br 144 -e " + files[i] + " " + tmpPath;
        system(command.c_str());
        if(!file_exists(tmpPath.c_str())){ // There was some error in conversion
        	return atracFiles;
        }
        FILE* tmpFile = fopen(tmpPath.c_str(), "rb");
        atracFiles.push_back(tmpFile);
	}

	return atracFiles;
}

void interlaceFiles(std::vector<FILE*> files, std::string outputPath){
	int index = outputPath.find_last_of('/');
	std::error_code error;
	if(index != std::string::npos){
		if(!createDirectoryRecursive(outputPath.substr(0, index).c_str(), error)){
			std::cout << "ERROR: Could not create directory for destination file: " << error << std::endl;
			return;
		}
    }

    FILE* yuk = fopen(outputPath.c_str(), "wb");
    if(yuk == NULL){
    	std::cout << "ERROR: Could not create destination file " << outputPath << std::endl;
    	return;
    }

    // Calculate total filesize
    size_t fileSize = 0;
    for(int i = 0; i < files.size(); i++){
    	fseek(files[i], 0, SEEK_END);
    	fileSize += ftell(files[i]) - 464;
    	fseek(files[i], 464, SEEK_SET);
    }


    std::cout << "Calculating chunks" << std::endl;

    size_t chunkSize = 0x30000; // 0x6000 size * 8 streams

    int numChunks = fileSize / chunkSize;
    int extraChunks1 = 0;
    int extraChunks2 = 0;

    int remainder = fileSize % chunkSize;
    if(remainder != 0){
        numChunks--;
        int remainingChunks = remainder / 0xC00;
        extraChunks1 = remainingChunks / 2;
        extraChunks2 = remainingChunks - extraChunks1;
    }

    std::cout << "Writing " << numChunks << " major chunks" << std::endl;

    char buffer[0x6000];
    for(int i = 0; i < numChunks; i++){
        for(int j = 0; j < 8; j++){
            fread(buffer, sizeof(char), 0x6000, files[j]);
            fwrite(buffer, sizeof(char), 0x6000, yuk);
        }
    }

    std::cout << "Writing " << extraChunks1 + extraChunks2 << " minor chunks" << std::endl;

    for(int i = 0; i < 8; i++){
        fread(buffer, sizeof(char), extraChunks1 * 0x180, files[i]);
        fwrite(buffer, sizeof(char), extraChunks1 * 0x180, yuk);
    }
    for(int i = 0; i < 8; i++){
        fread(buffer, sizeof(char), extraChunks2 * 0x180, files[i]);
        fwrite(buffer, sizeof(char), extraChunks2 * 0x180, yuk);
    }

    std::cout << "Interlacing complete" << std::endl;

    for(int i = 0; i < files.size(); i++){
        fclose(files[i]);
    }

    fclose(yuk);
}

void cleanupFiles(std::vector<std::string> files){
	for(int i = 0; i < files.size(); i++){
		std::string tmpPath = files[i] + "_" + std::to_string(i);
		std::remove(tmpPath.c_str());
	}
}

int main(int argc, char* argv[]){
	std::cout << "Full Auto 2 .yuk Music Compressor by protorizer" << std::endl;

	if(!file_exists("./at3tool/ps3_at3tool.exe")){
        std::cout << "at3tool not found. Please download at3tool and place it in a subfolder named \"at3tool\"." << std::endl;
        return 0;
    }

    if(argc != 3){
        std::cout << "Invalid parameters. Usage: " << argv[0] << " [input directory] [output file]" << std::endl;
        return 0;
    }

    std::string inputDir = argv[1];
    std::string outputFile = argv[2];

    std::cout << "Reading files" << std::endl;
    std::vector<std::string> files = readFiles(inputDir);

    if(files.size() != 8){
    	return 0;
    }

    std::cout << "Converting files" << std::endl;
    std::vector<FILE*> atracFiles = convertFiles(files);

    if(atracFiles.size() < 8){
    	return 0;
    }

    std::cout << "Interlacing files" << std::endl;
    interlaceFiles(atracFiles, outputFile);

    std::cout << "Removing temporary files" << std::endl;
    cleanupFiles(files);

    std::cout << "Done!" << std::endl;

	return 0;
}