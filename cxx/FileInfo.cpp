/*
 * Copyright (c) 2015, Shinya Yagyu
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, 
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its 
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include <assert.h>
#include <stdio.h>
#include "Storjutp.h"


FileInfo::FileInfo(){
    handler = NULL;
    socket = NULL;
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

size_t SendFileInfo::getByte(byte* buf){
    unsigned char header[40];
    size2header(header);
    size_t buf_pos = 0;
    if(loc < 40){
        for(; loc < 40 ;buf_pos++, loc++){
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

size_t SendFileInfo::getProgress(){
    return ftell(fp);
}


ReceiveFileInfo::ReceiveFileInfo(){
    fp = NULL;
    size = 0;
 }

ReceiveFileInfo::~ReceiveFileInfo(){
    if(fp) fclose(fp);
}

bool ReceiveFileInfo::isCompleted(){
    return size == (uint64_t)ftell(fp);
}

size_t ReceiveFileInfo::getProgress(){
    return ftell(fp);
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

size_t UnknownFileInfo::getProgress(){
    return 0;
}
