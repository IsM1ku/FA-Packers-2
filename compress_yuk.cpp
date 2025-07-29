#include <iostream>
#include <filesystem>
#include <string>
#include <cstdio>
#include <vector>

std::vector<std::vector<char>> g_headers;

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
        g_headers.clear();
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

                std::string headerPath = inputDir + "/" + inputDir + "_Stream" + std::to_string(i) + ".hdr";
                FILE* h = fopen(headerPath.c_str(), "rb");
                if(!h){
                        std::cout << "Error: header file " << headerPath << " not found." << std::endl;
                        files.clear();
                        return files;
                }
                std::vector<char> hdr(464);
                fread(hdr.data(), sizeof(char), 464, h);
                fclose(h);
                g_headers.push_back(hdr);
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
        if(!tmpFile){
                return atracFiles;
        }
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

    // Determine stream length from first file
    fseek(files[0], 0, SEEK_END);
    size_t dataSize = ftell(files[0]) - 464; // skip encoder header
    fseek(files[0], 464, SEEK_SET);

    const size_t frameSize = 0x180;
    const size_t chunkData = 0x6000;

    if(dataSize % frameSize != 0){
        std::cout << "Warning: stream size not frame aligned, trailing bytes will be ignored" << std::endl;
    }
    size_t frames = dataSize / frameSize;
    size_t numChunks = frames / 64; // 64 frames per 0x6000 block
    size_t extraFrames = frames % 64;

    std::cout << "Writing " << numChunks << " major chunks" << std::endl;

    char buffer[0x6000];
    for(size_t c = 0; c < numChunks; c++){
        for(int j = 0; j < 8; j++){
            if(c == 0){
                fwrite(g_headers[j].data(), sizeof(char), 464, yuk);
                fread(buffer, sizeof(char), chunkData - 464, files[j]);
                fwrite(buffer, sizeof(char), chunkData - 464, yuk);
            }else{
                fread(buffer, sizeof(char), chunkData, files[j]);
                fwrite(buffer, sizeof(char), chunkData, yuk);
            }
        }
    }

    char frameBuf[0x180];
    std::cout << "Writing " << extraFrames << " leftover frames" << std::endl;
    for(size_t f = 0; f < extraFrames; f++){
        for(int j = 0; j < 8; j++){
            fread(frameBuf, sizeof(char), frameSize, files[j]);
            fwrite(frameBuf, sizeof(char), frameSize, yuk);
        }
    }

    size_t totalSize = (464 + frames * frameSize) * 8;
    std::cout << "Total file size: " << totalSize << " bytes" << std::endl;

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