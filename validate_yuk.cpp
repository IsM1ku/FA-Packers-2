#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <ctime>

struct ValidationResult {
    long dataSize = 0;
    long frames = 0;
    long remainder = 0;
    bool headerOk = true;
    bool sizeOk = true;
    bool frameAligned = true;
};

static void logLine(const std::string& msg, std::ofstream* log)
{
    std::cout << msg << std::endl;
    if(log && log->is_open())
        (*log) << msg << std::endl;
}

bool validateFile(const std::string& path, bool rawMode, ValidationResult& result, std::ofstream* log){
    FILE* f = fopen(path.c_str(), "rb");
    if(!f){
        logLine("File " + path + " not found", log);
        result.headerOk = result.sizeOk = result.frameAligned = false;
        return false;
    }

    char hdr[4] = {0};
    bool headerOk = true;
    bool sizeOk = true;
    if(!rawMode){
        if(fread(hdr,1,4,f) != 4){
            fseek(f,0,SEEK_END);
            long actual = ftell(f);
            logLine(path + ": cannot read RIFF header (file only " + std::to_string(actual) + " bytes)", log);
            fclose(f);
            result.headerOk = result.sizeOk = result.frameAligned = false;
            return false;
        }
        headerOk = hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F';
    }

    fseek(f,0,SEEK_END);
    long size = ftell(f);
    if(!rawMode && size < 464){
        logLine(path + ": file size " + std::to_string(size) + " bytes is smaller than expected 464 byte header", log);
        sizeOk = false;
    }
    long dataSize = rawMode ? size : (size > 464 ? size - 464 : 0);
    bool frameAligned = (dataSize % 0x180) == 0;
    long remainder = dataSize % 0x180;
    fclose(f);

    if(!rawMode && !headerOk){
        std::ostringstream oss;
        oss << path << ": invalid RIFF header. Found 0x"
            << std::hex << std::uppercase
            << std::setw(2) << std::setfill('0') << (int)(unsigned char)hdr[0]
            << std::setw(2) << (int)(unsigned char)hdr[1]
            << std::setw(2) << (int)(unsigned char)hdr[2]
            << std::setw(2) << (int)(unsigned char)hdr[3]
            << std::dec << ", expected 'RIFF'";
        logLine(oss.str(), log);
    }
    if(!frameAligned){
        logLine(path + ": data size after header " + std::to_string(dataSize) +
                " bytes is not a multiple of 0x180 (remainder " +
                std::to_string(remainder) + ")", log);
    }

    result.dataSize = dataSize;
    result.frames = dataSize / 0x180;
    result.remainder = remainder;
    result.headerOk = headerOk;
    result.sizeOk = sizeOk;
    result.frameAligned = frameAligned;

    std::ostringstream info;
    info << path << ": " << dataSize << " bytes, " << result.frames << " frames";
    if(remainder != 0)
        info << " (remainder " << remainder << ")";
    logLine(info.str(), log);

    return headerOk && frameAligned && sizeOk;
}

int main(int argc,char* argv[]){
    bool rawMode = false;
    std::string logName;

    if(argc < 2){
        std::cout << "Usage: " << argv[0] << " [stream prefix] [--raw] [--log <file>]" << std::endl;
        return 1;
    }

    std::string prefix = argv[1];
    for(int i = 2; i < argc; ++i){
        std::string arg = argv[i];
        if(arg == "--raw"){
            rawMode = true;
        }else if(arg == "--log"){
            if(i + 1 >= argc){
                std::cout << "--log requires a filename" << std::endl;
                return 1;
            }
            logName = argv[++i];
        }else{
            std::cout << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }

    std::ofstream logFile;
    if(!logName.empty()){
        logFile.open(logName);
        if(!logFile){
            std::cout << "Failed to open log file: " << logName << std::endl;
            return 1;
        }
    }

    {
        char buf[32];
        std::time_t now = std::time(nullptr);
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        logLine(std::string("Validation started: ") + buf, &logFile);
    }

    bool ok = true;
    std::vector<ValidationResult> results(8);
    for(int i=0;i<8;i++){
        std::string file = prefix + "_Stream" + std::to_string(i) + ".atrac";
        if(!validateFile(file, rawMode, results[i], &logFile))
            ok = false;
    }

    logLine("Summary:", &logFile);
    for(int i=0;i<8;i++){
        const ValidationResult& r = results[i];
        std::ostringstream oss;
        bool pass = r.headerOk && r.frameAligned && r.sizeOk;
        oss << "Stream" << i << ": " << r.dataSize << " bytes, " << r.frames << " frames ";
        if(pass){
            oss << "\xE2\x9C\x93"; // check mark
        }else{
            oss << "\xE2\x9C\x97"; // cross mark
            if(!r.frameAligned && r.remainder){
                oss << " (remainder " << r.remainder << ")";
            }
        }
        logLine(oss.str(), &logFile);
    }

    if(ok)
        logLine("All streams validated successfully.", &logFile);

    return ok?0:1;
}

