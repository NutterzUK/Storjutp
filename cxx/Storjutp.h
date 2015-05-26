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

#ifndef _Storjutp_H_
#define _Storjutp_H_

#include <vector>  
#include <string>  
#include <list>  
#include <map>  

#include "utp.h"


using namespace std;

/**
 * Class for handling  received packets in telehash-c channel.
 * 
 * handle() method is called per one packet,
 */
class Handler{

public:
    /**
     * handle one packet.
     * 
     * @param json one packet described in json.
     * @return a json packet that should be sent back. If NULL, no packets 
     *          are sent back.
     */
	virtual void on_finish(unsigned char hash[32], 
                           const char *errMessage)=0;
    
    /**
     * destructor
     */
    virtual ~Handler(){}
};

struct FileInfo {
    bool isSending;
    FILE *fp;
    unsigned char hash[32];
    Handler *handler;
    utp_socket *socket;
};



 /**
  * class for managing telehash-c.
  * 
  * Everything is not thread safe. Call a function after stopoping
  *  start() loop by setStopFlag(1).
  * It is NOT recommended to make more than one instance. If you want to do so
  * (e.g. for test use), use setGC(0) to stop GC.
  */
class Storjutp{
private:
     utp_context * ctx;
    /**
     * stop flag while looping to read packets.
     */
    int stopFlag;

public:
    int server_port;
    int fd;
    list<FileInfo *> fileInfos;

    /**
     * constructor.
     * 
     * If id.json including hashname is not existed,it will be created.
     * Instances are always created based on id.json except port=-9999.
     * Never use port=-9999 except for test use.
     * 
     * @param port port number to be listened packets. if 0, port number is 
     *         selected randomly.
     * @param factory ChannelHanderFactory instance.
     * @param broadcastHandler ChannelHandler instnace for handlinkg broadcasts 
     */
    Storjutp(int port = 0);
    void deleteFileInfo(FileInfo *fi);
    void registHash(unsigned char* hash, Handler *handler);
    void unregistHash(unsigned char* hash);
    int sendFile(string dest, int port, string fname, unsigned char *hash,
                 Handler *handler);

    /**
     * destructor.
     * free some telehash-c stuffs.
     */
    ~Storjutp();

    /**
     * start receiving packet loop.
     * call setStopFlag(1) when you want to stop.
     * 
     */
    void start();

    /**
     * set stopFlag that stop/continue a loop..
     *
     * @param flag 1 if stop. others if run.
     */
    void setStopFlag(int flag);
};
#endif
