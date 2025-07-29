#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <iomanip>

bool validateFile(const std::string& path, bool rawMode){
    FILE* f = fopen(path.c_str(), "rb");
    if(!f){
        std::cout << "File " << path << " not found" << std::endl;
        return false;
    }

    char hdr[4] = {0};
    bool headerOk = true;
    bool sizeOk = true;
    if(!rawMode){
        if(fread(hdr,1,4,f) != 4){
            fseek(f,0,SEEK_END);
            long actual = ftell(f);
            std::cout << path << ": cannot read RIFF header (file only " << actual << " bytes)" << std::endl;
            fclose(f);
            return false;
        }
        headerOk = hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F';
    }

    fseek(f,0,SEEK_END);
    long size = ftell(f);
    if(!rawMode && size < 464){
        std::cout << path << ": file size " << size << " bytes is smaller than expected 464 byte header" << std::endl;
        sizeOk = false;
    }
    long dataSize = rawMode ? size : (size > 464 ? size - 464 : 0);
    bool frameAligned = (dataSize % 0x180) == 0;
    long remainder = dataSize % 0x180;
    fclose(f);

    if(!rawMode && !headerOk){
        std::ios state(nullptr);
        state.copyfmt(std::cout);
        std::cout << path << ": invalid RIFF header. Found 0x"
                  << std::hex << std::uppercase
                  << std::setw(2) << std::setfill('0') << (int)(unsigned char)hdr[0]
                  << std::setw(2) << (int)(unsigned char)hdr[1]
                  << std::setw(2) << (int)(unsigned char)hdr[2]
                  << std::setw(2) << (int)(unsigned char)hdr[3]
                  << std::dec << ", expected 'RIFF'" << std::endl;
        std::cout.copyfmt(state);
    }
    if(!frameAligned){
        std::cout << path << ": data size after header " << dataSize
                  << " bytes is not a multiple of 0x180 (remainder "
                  << remainder << ")" << std::endl;
    }

    return headerOk && frameAligned && sizeOk;
}

int main(int argc,char* argv[]){
    bool rawMode = false;
    if(argc < 2 || argc > 3){
        std::cout << "Usage: " << argv[0] << " [stream prefix] [--raw]" << std::endl;
        return 1;
    }
    if(argc == 3){
        std::string flag = argv[2];
        if(flag == "--raw")
            rawMode = true;
        else{
            std::cout << "Unknown option: " << flag << std::endl;
            return 1;
        }
    }
    std::string prefix = argv[1];
    bool ok = true;
    for(int i=0;i<8;i++){
        std::string file = prefix + "_Stream" + std::to_string(i) + ".atrac";
        if(!validateFile(file, rawMode))
            ok = false;
    }
    if(ok)
        std::cout << "All streams validated successfully." << std::endl;
    return ok?0:1;
}

