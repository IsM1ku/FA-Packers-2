#include <iostream>
#include <cstdio>
#include <string>
#include <vector>

bool validateFile(const std::string& path){
    FILE* f = fopen(path.c_str(), "rb");
    if(!f){
        std::cout << "File " << path << " not found" << std::endl;
        return false;
    }
    char hdr[4];
    if(fread(hdr,1,4,f) != 4){
        std::cout << path << ": cannot read header" << std::endl;
        fclose(f);
        return false;
    }
    bool headerOk = hdr[0]=='R' && hdr[1]=='I' && hdr[2]=='F' && hdr[3]=='F';
    fseek(f,0,SEEK_END);
    long size = ftell(f);
    long dataSize = size > 464 ? size - 464 : 0;
    bool frameAligned = (dataSize % 0x180) == 0;
    fclose(f);

    if(!headerOk)
        std::cout << path << ": invalid ATRAC header" << std::endl;
    if(!frameAligned)
        std::cout << path << ": data size not frame aligned" << std::endl;
    return headerOk && frameAligned;
}

int main(int argc,char* argv[]){
    if(argc!=2){
        std::cout << "Usage: " << argv[0] << " [stream prefix]" << std::endl;
        return 1;
    }
    std::string prefix = argv[1];
    bool ok = true;
    for(int i=0;i<8;i++){
        std::string file = prefix + "_Stream" + std::to_string(i) + ".atrac";
        if(!validateFile(file))
            ok = false;
    }
    if(ok)
        std::cout << "All streams validated successfully." << std::endl;
    return ok?0:1;
}

