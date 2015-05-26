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
public:
    /**
     * create myself with handlers.
     * 
     * @param h channels.
     */
	TestHandler(bool isSender){
        this->isSender = isSender;
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
        ok( !error, "no error on sending.");
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


void sendingTest(){
    unsigned char dummy[32];
    int i = 0;
    string fname;
    
    for(i=0;i<32;i++){
        dummy[i]=i;
        char c[3];
        sprintf(c, "%02X", i);
        fname += c;
    }
    Storjutp s1(12345);
    LOG("%d",s1.server_port);
    ok(s1.server_port == 12345, "port number check");
    TestHandler te1(false);
    s1.registHash(dummy, &te1);

    Storjutp s2;
    TestHandler te2(true);
    int r = s2.sendFile("127.0.0.1", 12345, 
               "tests/nonexist.dat", dummy, &te2);   
    ok(r, "prepare to send non-exist file check");

    s2.sendFile("127.0.0.1", 12345, 
               "tests/rand.dat", dummy, &te2);   

    thread t1=std::thread([&](){
        s1.start();
    });
    thread t2=std::thread([&](){
        s2.start();
    });
    t1.join();
    t2.join();
    LOG("fname = %s", fname.c_str());
    ok(isSame("tests/rand.dat", fname), "file equality between original"
                                         " and copyied check");
}

int main (int argc, char *argv[]) {
    sendingTest();
    done_testing();

}
