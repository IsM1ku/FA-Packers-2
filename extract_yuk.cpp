#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdio>

bool file_exists(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    bool is_exist = false;
    if (fp != NULL)
    {
        is_exist = true;
        fclose(fp);
    }
    return is_exist;
}

bool createDirectoryRecursive(std::string const & dirName, std::error_code & err)
{
    err.clear();
    if (!std::filesystem::create_directories(dirName, err))
    {
        if (std::filesystem::exists(dirName))
        {
            err.clear();
            return true;
        }
        return false;
    }
    return true;
}

std::vector<std::string> extractStreams(FILE* file, const std::string& outputPath, std::vector<size_t>& streamSizes){
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

    const size_t chunkSize = 0x30000;
    const size_t frameSize = 0x180;
    const size_t streamBlockSize = 0x6000;

    size_t numChunks = fileSize / chunkSize;
    size_t remainder = fileSize % chunkSize;
    size_t totalExtraFrames = remainder / frameSize;
    size_t leftoverGarbage = remainder % frameSize;

    if(leftoverGarbage != 0){
        std::cout << "Warning: " << leftoverGarbage
                  << " trailing bytes do not form a full frame and will be ignored." << std::endl;
    }

    char buffer[0x6000];
    for(size_t i = 0; i < numChunks; i++){
        for(int j = 0; j < 8; j++){
            fread(buffer, sizeof(char), streamBlockSize, file);
            if(i == 0){
                fwrite(buffer, sizeof(char), 464, headerStreams[j]);
            }
            fwrite(buffer, sizeof(char), streamBlockSize, outputStreams[j]);
        }
    }

    char frameBuf[0x180];
    std::vector<size_t> frameCount(8, 0);
    for(size_t f = 0; f < totalExtraFrames; ++f){
        int streamIndex = f % 8;
        fread(frameBuf, sizeof(char), frameSize, file);
        fwrite(frameBuf, sizeof(char), frameSize, outputStreams[streamIndex]);
        frameCount[streamIndex]++;
    }

    for(int i = 0; i < 8; i++){
        fclose(outputStreams[i]);
        fclose(headerStreams[i]);
        std::cout << "Stream " << i << " total extra frames: " << frameCount[i] << std::endl;
        streamSizes.push_back(std::filesystem::file_size(outputFiles[i]));
    }

    size_t perStreamSize = numChunks * streamBlockSize + (totalExtraFrames / 8) * frameSize;
    std::cout << "File size: " << fileSize << " bytes" << std::endl;
    std::cout << "Per-stream size (min): " << perStreamSize << " bytes" << std::endl;

    return outputFiles;
}

void writeHeaderInfo(const std::vector<std::string>& streamPaths, const std::vector<size_t>& streamSizes){
    for(size_t i = 0; i < streamPaths.size(); ++i){
        std::string filePath = streamPaths[i] + ".txth";
        FILE* file = fopen(filePath.c_str(), "w");
        if(!file) continue;
        size_t numSamples = (streamSizes[i] / 0x180) * 1024;
        std::string headerInfo =
            "codec = ATRAC3\n"
            "sample_rate = 48000\n"
            "channels = 2\n"
            "start_offset = 0\n"
            "interleave = 0x180\n"
            "frame_size = 0x180\n"
            "samples_per_frame = 1024\n"
            "num_samples = " + std::to_string(numSamples);
        fprintf(file, "%s", headerInfo.c_str());
        fclose(file);
    }
}

void convertStreams(std::vector<std::string> streamPaths, std::string outputPath){
    for(int i = 0; i < streamPaths.size(); i++){
        std::string command = ".\\vgmstream\\vgmstream-cli.exe -o " + outputPath + "_Stream" + std::to_string(i) + ".wav " + streamPaths[i];
        system(command.c_str());
    }
}

int main(int argc, char* argv[]){
    std::cout << "Full Auto 2 .yuk Music Extractor (Final Fix)" << std::endl;

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
    std::vector<size_t> streamSizes;
    std::vector<std::string> streamPaths = extractStreams(file, atracPath, streamSizes);
    fclose(file);

    std::cout << "Writing codec .txth file" << std::endl;
    writeHeaderInfo(streamPaths, streamSizes);

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