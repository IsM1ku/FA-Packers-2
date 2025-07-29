#include <iostream>
#include <filesystem>
#include <string>
#include <vector>

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

std::vector<std::string> extractStreams(FILE* file, std::string outputPath){
    std::vector<std::string> outputFiles;
    std::vector<FILE*> outputStreams;
    std::vector<FILE*> headerStreams;

    for(int i = 0; i < 8; i++){
        outputFiles.push_back(outputPath + "_Stream" + std::to_string(i) + ".atrac");
        std::cout << "Saving " << outputFiles[i] << std::endl;
        outputStreams.push_back(fopen(outputFiles[i].c_str(), "wb"));
        std::string headerPath = outputPath + "_Stream" + std::to_string(i) + ".hdr";
        headerStreams.push_back(fopen(headerPath.c_str(), "wb"));
    }

    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    const size_t chunkSize = 0x30000; // 0x6000 size * 8 streams
    const size_t frameSize = 0x180;

    size_t numChunks = fileSize / chunkSize;
    size_t remainder = fileSize % chunkSize;
    size_t extraFrames = remainder / (frameSize * 8);
    size_t leftoverBytes = remainder % (frameSize * 8);
    if(leftoverBytes != 0){
        std::cout << "Warning: file remainder " << leftoverBytes
                  << " bytes is not frame aligned and will be ignored" << std::endl;
    }

    char buffer[0x6000];
    for(size_t i = 0; i < numChunks; i++){
        for(int j = 0; j < 8; j++){
            fread(buffer, sizeof(char), 0x6000, file);
            if(i == 0){
                // Save the encoder header separately but also keep it in the output
                fwrite(buffer, sizeof(char), 464, headerStreams[j]);
            }
            fwrite(buffer, sizeof(char), 0x6000, outputStreams[j]);
        }
    }

    char frameBuf[0x180];
    for(size_t f = 0; f < extraFrames; f++){
        for(int j = 0; j < 8; j++){
            fread(frameBuf, sizeof(char), frameSize, file);
            fwrite(frameBuf, sizeof(char), frameSize, outputStreams[j]);
        }
    }

    for(int i = 0; i < 8; i++){
        fclose(outputStreams[i]);
        fclose(headerStreams[i]);
    }

    size_t perStreamSize = numChunks * 0x6000 + extraFrames * frameSize;
    std::cout << "File size: " << fileSize << " bytes" << std::endl;
    std::cout << "Per-stream size: " << perStreamSize << " bytes" << std::endl;

    return outputFiles;
}

void writeHeaderInfo(std::string outputDir){
    std::string filePath = outputDir + "/atrac/" + ".atrac.txth";
    FILE* file = fopen(filePath.c_str(), "w");
    std::string headerInfo = "codec = ATRAC3\nsample_rate = 48000\nchannels = 2\nstart_offset = 0\ninterleave = 0x180\nnum_samples = data_size";
    fprintf(file, "%s", headerInfo.c_str());
    fclose(file);
}

void convertStreams(std::vector<std::string> streamPaths, std::string outputPath){
    for(int i = 0; i < streamPaths.size(); i++){
        std::string command = ".\\vgmstream\\vgmstream-cli.exe -o " + outputPath + "_Stream" + std::to_string(i) + ".wav " + streamPaths[i];
        system(command.c_str());
    }
}

int main(int argc, char* argv[]){
	std::cout << "Full Auto 2 .yuk Music Extractor by protorizer" << std::endl;

    if(!file_exists("./vgmstream/vgmstream-cli.exe")){
        std::cout << "vgmstream not found. Please download vgmstream and place it in a subfolder named \"vgmstream\"." << std::endl;
        return 0;
    }

    if(argc != 3){
        std::cout << "Invalid parameters. Usage: " << argv[0] << " [input file] [output directory]" << std::endl;
        return 0;
    }

    std::string inputFile = argv[1];
    std::string outputDir = argv[2];

    FILE* file = fopen(inputFile.c_str(), "rb");
    if(file == NULL){
        std::cout << "File " << inputFile << " not found." << std::endl;
        return 0;
    }

    std::error_code error;
    if(!createDirectoryRecursive(outputDir + "/atrac/", error) || !createDirectoryRecursive(outputDir + "/wav/", error)){
        std::cout << "Could not create output directory, error: " << error.message() << std::endl;
        return 0;
    }

    std::string atracPath = outputDir + "/atrac/" + inputFile;
    int index = atracPath.find_last_of('.');
    if(index != std::string::npos){
        atracPath = atracPath.substr(0, index);
    }

    std::cout << "Extracting ATRAC streams" << std::endl;
    std::vector<std::string> streamPaths = extractStreams(file, atracPath);
    fclose(file);

    std::cout << "Writing codec .txth file" << std::endl;
    writeHeaderInfo(outputDir);

    std::string wavPath = outputDir + "/wav/" + inputFile;
    index = wavPath.find_last_of('.');
    if(index != std::string::npos){
        wavPath = wavPath.substr(0, index);
    }

    std::cout << "Converting to .wav" << std::endl;
    convertStreams(streamPaths, wavPath);

    std::cout << "Done!" << std::endl;

	return 0;
}