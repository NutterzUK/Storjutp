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

#include "Storjutp.h"

#include <unistd.h>
#include <tap.h>
#include <vector>  
#include <thread>
#include <string.h>

using namespace std;

#define LOG(fmt, ...) log_(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
void *log_(const char *file, int line, const char *function,
                        const char * format, ...) {
  char buffer[256];
  va_list args;
  va_start (args, format);
  vsnprintf (buffer, 256, format, args);
  fprintf(stderr,"%s:%d %s() %s\n", file, line, function, buffer);
  fflush(stderr);
  va_end (args);
  return NULL;
}

class TestHandler : public Handler{
private:
    bool isSender;
    bool hasError;
public:
    bool finished;
    /**
     * create myself with handlers.
     * 
     * @param h channels.
     */
	TestHandler(bool isSender, bool hasError = false){
        this->isSender = isSender;
        this->hasError = hasError;
        finished = false;
	}
	
    /**
     * handle one packet.
     * 
     * @param json one packet described in json.
     * @return a json packet that should be sent back. not be sent if NULL.
     */
	void on_finish(unsigned char *hash, const char *error){
        LOG("isSender = %d", isSender);
        if(error){
            LOG("error: %s",  error);
        }
        LOG("finished");
        if(hasError){
            ok( error!=NULL, "error occured on sending.");
        }else{
            ok( error==NULL, "no error on sending.");
        }
        finished = true;
	}
    
    ~TestHandler(){
    }
};

bool isSame(string file1,string file2){
    FILE *f1=fopen(file1.c_str(),"rb");
    FILE *f2=fopen(file2.c_str(),"rb");

    if(f1==NULL || f2==NULL){
      return false;
    }
    int c1='\0';
    int c2='\0';
    bool same=true;
    while( (c1=fgetc(f1))!=EOF){
        c2=fgetc(f2);
        if(c1!=c2){
            LOG("data differs");
            same=false;
            break;
        }
    }
    if( (c1=fgetc(f1))!=EOF || (c2=fgetc(f2))!=EOF){
        LOG("size differs");
        LOG("%x %x",c1,c2);
        same=false;
    }
    fclose(f1);
    fclose(f2);
    return same;
}

void generateHash(unsigned char hash[32], string& str){
    for(int i=0;i<32;i++){
        hash[i]=i;
        char c[3];
        sprintf(c, "%02X", i);
        str += c;
    }
}


void errorTest(){
    LOG("starting errorTest");
    unsigned char dummy[32], dummy2[32];
    int i = 0, j=0;
    string fname, fname2;
    
    generateHash(dummy, fname);
    unlink(fname.c_str());
    for(i=31,j=0;i>=0;i--,j++){
        dummy2[j]=i;
        char c[3];
        sprintf(c, "%02X", i);
        fname2 += c;
    }
    unlink(fname2.c_str());
    int r=0;
    
    Storjutp s1(12345);
    LOG("server port = %d",s1.server_port);
    ok(s1.server_port == 12345, "port number check");
    Storjutp s3(12345);
    ok(s3.server_port == 12346, "port number check2");

    TestHandler te1(false, true);
    s1.registHash(dummy, &te1);

    r = s1.registHash(dummy, &te1);
    ok(r, "double regist test");

    s1.unregistHash(dummy);
    r = s1.registHash(dummy, &te1);
    ok(!r, "re-regist after unregist test");

    Storjutp s2;
    TestHandler te2(true, true);

    r = s2.sendFile("1277.1.1.1.1", 12345, 
               "tests/rand.dat", dummy, &te2);   
    ok(r, "prepare to send to error address check");



    r = s2.sendFile("127.0.0.1", 12345, 
               "tests/nonexist.dat", dummy, &te2);   
    ok(r, "prepare to send non-exist file check");

    r = s2.sendFile("127.0.0.1", 12345, 
               "tests/rand.dat", dummy2, &te2);   
    ok(!r, "start to send file test");

    thread t1=std::thread([&](){
        s1.start();
    });
    thread t2=std::thread([&](){
        s2.start();
    });
    sleep(10);
    s1.setStopFlag(1);
    s2.setStopFlag(1);
    t1.join();
    t2.join();
    FILE *fp = fopen(fname2.c_str(),"rb");
    ok(!fp, "not registered file test");
    ok(!te1.finished, "finish check 1");
    ok(te2.finished, "finish check 2");

    r = s2.sendFile("127.0.0.1", 66666, 
               "tests/rand.dat", dummy2, &te2);   
    ok(!r, "prepare to send file after finish check");
    t2=std::thread([&](){
        s2.start();
    });
    sleep(10);
    s2.setStopFlag(1);
    t2.join();
    ok(te2.finished, "finish check 2");
}

void sendTest(){
    LOG("starting sendTest");
    unsigned char dummy[32];
    string fname;
    
    generateHash(dummy, fname);
    unlink(fname.c_str());
    Storjutp s1(12345);
    LOG("%d",s1.server_port);
    TestHandler te1(false);
    s1.registHash(dummy, &te1);

    Storjutp s2;
    TestHandler te2(true);
    s2.sendFile("127.0.0.1", s1.server_port, 
               "tests/rand.dat", dummy, &te2);   

    thread t1=std::thread([&](){
        s1.start();
    });
    thread t2=std::thread([&](){
        s2.start();
    });
    sleep(10);
    s1.setStopFlag(1);
    s2.setStopFlag(1);
    t1.join();
    t2.join();
    LOG("fname = %s", fname.c_str());
    ok(isSame("tests/rand.dat", fname), "file equality between original"
                                         " and copyied check");
    ok(te1.finished, "finish check 1");
    ok(te2.finished, "finish check 2");
}

bool checkArray(unsigned char *a, unsigned char *b, size_t len){
    for(size_t i = 0;i<len;i++){
        if(a[i]!=b[i]) return false;
    }
    return true;
}

void fileInfoTest(){
    unsigned char dummy[32];
    unsigned char buf[40];
    string fname;

    generateHash(dummy, fname);

    SendFileInfo fi;
    FILE *fp =NULL;
    fp = fopen("tests/rand.dat", "rb");
    fi.setFP(fp);
    ok(fi.size == 25424239, "size in SendFileInfo test");
    
    memcpy(fi.hash, dummy, 32);

    fi.getByte(buf, 30);
    fi.seek(-5);
    fi.getByte(buf+25, 15);
    ok(checkArray(buf, dummy, 32), "buf in SendFileInfo test 1");
    unsigned char r[8];
    r[0]=0x0;
    r[1]=0x0;
    r[2]=0x0;
    r[3]=0x0;
    r[4]=0x01;
    r[5]=0x83;
    r[6]=0xF1;
    r[7]=0x6F;
    ok(checkArray(buf + 32, r, 8), "buf in SendFileInfo test 2");
    fclose(fp);
    fi.setFP(NULL);

    UnknownFileInfo ufi;
    for(int i=0;i<32;i++){
        buf[i] = dummy[i];
    }
    buf[32+0]=0x00;
    buf[32+1]=0x00;
    buf[32+2]=0x00;
    buf[32+3]=0x00;
    buf[32+4]=0x01;
    buf[32+5]=0x83;
    buf[32+6]=0xF1;
    buf[32+7]=0x6F;
    ufi.putByte(buf, 40);
    ok(checkArray(buf, dummy, 32), "buf in UnknownFileInfo test");
    LOG("size = %d",ufi.size);
    ok(ufi.size == 25424239,  "size in UnknownFileInfo test");

    dies_ok({ufi.isCompleted();} , "UnknownFileInfo isCompeted test");

}

int main (int argc, char *argv[]) {
    fileInfoTest();
    errorTest();
    sendTest();
    done_testing();
}
