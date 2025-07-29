#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

bool validateFile(const std::string& path, bool rawMode){
    FILE* f = fopen(path.c_str(), "rb");
    if(!f){
        std::cout << "File " << path << " not found" << std::endl;
        return false;
    }

    char hdr[4];
    bool headerOk = true;
    if(!rawMode){
        if(fread(hdr,1,4,f) != 4){
            std::cout << path << ": cannot read header" << std::endl;
            fclose(f);
            return false;
        }
        headerOk = hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F';
    }else{
        // In raw mode the ATRAC header is stored separately and may not contain
        // a standard RIFF wrapper. Skip header validation.
        headerOk = true;
    }

    fseek(f,0,SEEK_END);
    long size = ftell(f);
    long dataSize = rawMode ? size : (size > 464 ? size - 464 : 0);
    bool frameAligned = (dataSize % 0x180) == 0;
    fclose(f);

    if(!headerOk)
        std::cout << path << ": invalid ATRAC header" << std::endl;
    if(!frameAligned)
        std::cout << path << ": data size not frame aligned" << std::endl;
    return headerOk && frameAligned;
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

