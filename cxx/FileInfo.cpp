#include <assert.h>
#include "Storjutp.h"


FileInfo::FileInfo(){
    handler = NULL;
 }
    
FileInfo::~FileInfo(){
}

bool FileInfo::equal(unsigned char h[32]){
    for(int j=0;j<32;j++){
        if(h[j] != hash[j]){
            return false;
        }
    }
    return true;
}



SendFileInfo::SendFileInfo(){
    fp = NULL;
    loc = 0;
    size = 0;
 }

SendFileInfo::~SendFileInfo(){
    if(fp) fclose(fp);
}

void SendFileInfo::size2header(unsigned char header[40]){
    for(int i = 0;i < 32; i++){
        header[i] = hash[i];
    }
    for(int i = 32;i < 40; i++){
        int b =   (8 - (i - 31)) << 3;
        header[i] = (unsigned char) ((size >> b) & 0xff);
    }
}

void SendFileInfo::setFP(FILE *fp){
    this->fp = fp;
    if(fp){
        fseek(fp, 0, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
    }
}

size_t SendFileInfo::getByte(byte* buf, size_t len){
    unsigned char header[40];
    size2header(header);
    size_t buf_pos = 0;
    if(loc < 40){
        for(; len > 0 && loc < 40 ;buf_pos++, loc++, len--){
            buf[buf_pos] = header[loc];
        }
    }
    return buf_pos;
}

FILE *SendFileInfo::getFP(){
    return fp;
}
    
void SendFileInfo::seek(size_t to){
    loc += to;
}

bool SendFileInfo::isCompleted(){
    return feof(fp);
}


ReceiveFileInfo::ReceiveFileInfo(){
    fp = NULL;
    size = 0;
 }

ReceiveFileInfo::~ReceiveFileInfo(){
    if(fp) fclose(fp);
}

bool ReceiveFileInfo::isCompleted(){
    size_t l = ftell(fp);
    fseek(fp, 0, SEEK_END);
    size_t _size = ftell(fp);
    fseek(fp, l, SEEK_SET);
    printf("downloaded %ld, target = %ld\n",_size, size);
    return _size == size;
}


UnknownFileInfo::UnknownFileInfo(){
    loc = 0;
 }

UnknownFileInfo::~UnknownFileInfo(){
}

void UnknownFileInfo::header2size(){
    for(int i = 0;i < 32; i++){
        hash[i] = header[i];
    }
    size = 0;
    for(int i = 32 ;i < 40; i++){
        int b =   (8 - (i - 31)) << 3;
        size += ((uint64_t)header[i]) << b;
    }
}
   
bool UnknownFileInfo::hasHash(){
    return loc == 40;
}
    
size_t UnknownFileInfo::putByte(const byte* b, size_t len){
    size_t buf_pos = 0;
    if(loc < 40){
        for(; len > 0 && loc < 40; buf_pos++, loc++, len--){
            header[loc] = b[buf_pos];
        }
    }
    if(loc == 40) header2size();
    return buf_pos;
}

bool UnknownFileInfo::isCompleted(){
    return false;
}
