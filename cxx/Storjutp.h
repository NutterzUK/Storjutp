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

#include <string>  
#include <list>  

#include "utp.h"


using namespace std;

/**
 * Class for handling  when finishing downloading/uploading.
 */
class Handler{

public:
    /**
     * handle on finish.
     * 
     * @param hash file hash which finished downloading/uploading.
     * @param errMessage string if error. NULL if no error.
     * @return if 1, this object will be deteted after on_finish()
     */
	virtual int on_finish(unsigned char hash[32], 
                           const char *errMessage)=0;
    
    /**
     * destructor
     */
    virtual ~Handler(){}
};

/**
 * super class for storing information of a file that is 
 * downloading/uploading/yet unchecked.
 */
class FileInfo {
public:
    /**
     * handler called when finish.
     */
    Handler *handler;

    /**
     * hash of this file.
     */
    unsigned char hash[32];

    /**
     * constructor.
     */
    FileInfo();

    /**
     * destructor.
     */
    virtual ~FileInfo();

    /**
     * socket this file is using.
     */
    utp_socket *socket;

    /**
     * whethter whole file is downloaded/sended.
     * @return true if completed.
     */
    virtual bool isCompleted() = 0;

    /**
     * check the hash this file
     * 
     * @param h hash to be checked.
     * @return true if this file's hash is h[32].
     */
    bool equal(unsigned char h[32]);

};

/**
 * class for storing information of a file that is sending. 
 */
class SendFileInfo  : public FileInfo {
private:
    /**
     * sending file pointer
     */
    FILE *fp;

    /**
     * header current sending location/.
     */
    size_t loc;

    /**
     * convert hash and size to a byte array in hash -> size order.
     * 
     * @param header buffer to be stored.
     */
    void size2header(unsigned char header[40]);

public:
    /**
     * file size to be sended
     */
    uint64_t size;

    /**
     * constructor.
     */
    SendFileInfo();

    /**
     * destructor.
     */
    virtual ~SendFileInfo();

    /**
     * set file pointer, and set file size to the size variable.
     * @param fp file pointer to be set.
     */
    void setFP(FILE *fp);

    /**
     * get rest header(hash + byte array representation of size) byte array.
     * 
     * @param buf buffer header is stored.
     * @return size that is stored.
     */
    size_t getByte(byte* buf);
    
    /**
     * get file pointer
     * @return fp file pointer
     */
    FILE * getFP();

    /**
     * seek header location.
     * 
     * @param to location to be set. if minus, location will be rewinded.
     */
    void seek(size_t to);
    
    /**
     * whethter whole file is sended.
     * @return true if completed.
     */
    virtual bool isCompleted();
    
    /**
     * get uploaded size.
     * 
     * @return uploaded file size
     */
    size_t getProgress();

};

/**
 * class for storing information of a file that is receiving. 
 */
class ReceiveFileInfo : public FileInfo {
private:
    /**
     * header(hash + byte array representation of size) byte array
     */
    unsigned char header[40];

public:
    /**
     * file size to be received.
     */
    uint64_t size;
    
    /**
     * receving file pointer
     */
    FILE *fp;

    /**
     * constructor.
     */
    ReceiveFileInfo();
    /**
     * destructor.
     */
    virtual ~ReceiveFileInfo();
    
    /**
     * whethter whole file is receiving.
     * @return true if completed.
     */
    virtual bool isCompleted();
    
    /**
     * get downloaded size.
     * 
     * @return downloaded file size
     */
    size_t getProgress();

};

/**
 * class for storing information of a file that is not confirmed.
 */
class UnknownFileInfo : public FileInfo {
private:
    /**
     * header(hash + byte array representation of size) byte array
     */
    unsigned char header[40];
    
    /**
     * header current sending location/.
     */
    size_t loc;

    /**
     * convert header byte array to hash + size.
     */
    void header2size();

public:
    /**
     * file size to be received.
     */
    uint64_t size;

    /**
     * constructor.
     */
    UnknownFileInfo();
    
    /**
     * destructor.
     */
    virtual ~UnknownFileInfo();
    
    /**
     * whethter header has completely received.
     * @return if true, header has completely received.
     */
    bool hasHash();
    
    /**
     * put data to header. can be called over twice.
     * after receiving all header, header will be
     * converted to header[40] .
     * 
     * @param b byte array to be written.
     * @param len length of b.
     * @return wirtten size.
     */
    size_t putByte(const byte* b, size_t len);
    
    /**
     * @return always false.
     */
    virtual bool isCompleted();
    
};

 /**
  * class for managing uTP file transfer.
  */
class Storjutp{
private:
    /**
     * utp context.
     */
     utp_context * ctx;
    /**
     * stop flag while looping to read packets.
     */
    int stopFlag;

public:
    /**
     * server port that is listening.
     */
    int server_port;
    
    /**
     * UDP file descriptor.
     */
    int fd;

    /**
     * available FileIonfos.
     */
    list<FileInfo *> fileInfos;

    /**
     * indicator for test mode.
     * normally 0.
     */
    int isTesting;

    /**
     * for test mode 1(timeout test).
     * normally 0.
     */
    bool forceNoResponse;

    /**
     * constructor.
     * setup uTP stuffs, and start listeninig port.
     * 
     * @param port port number to be listened . if 0, port number is 
     *         selected randomly.
     */
    Storjutp(int port = 0);
    
    /**
     * delete items fro FileInfos.
     * 
     * @param fi FileInfo to be deleted.
     */
    void deleteFileInfo(FileInfo *fi);
    
    /**
     * find a ReceiveFileInfo which hash hash from FileInfos.
     * 
     * @param hash file hash to be searched.
     * @return found FileInfo instnace. NULL if not found.
     */
    FileInfo *findFileInfo(unsigned char* hash);
    
    /**
     * register acceptable file hash.
     * 
     * @param hash acceputable file hash.
     * @param handler Handler called when finish receiving a file.
     * @param dir  directory where file will be saved.
     * @return 0 if success.
     */
    int registHash(unsigned char* hash, Handler *handler, string dir=".");
    
    /**
     * unregister a hash and stop sending/downloading  file.
     * 
     * @param hash acceputable file hash to be unregistered.
     */
    void stopHash(unsigned char hash[32]);
    
    /**
     * prepare to send a file.
     * 
     * @param dest destination ip address.
     * @param port destination port to be sent.
     * @param fname file name to be sent.
     * @param hash file hash.
     * @param handler Handler called when finishing uploading.
     */
    int sendFile(string dest, int port, string fname, unsigned char *hash,
                 Handler *handler);
                 
    /**
     * set test mode. test use only. Don't use it.
     * 
     * @param test if 0, not test mode. if 1, timeout test. if 2, test for 
     * getting header over twice.
     */
    void setTesting(int test);

    /**
     * destructor.
     * free some telehash-c stuffs.
     */
    ~Storjutp();

    /**
     * start receiving packet loop.
     * call setStopFlag(1) when you want to stop.
     */
    void start();

    /**
     * set stopFlag that stop/continue a loop..
     *
     * @param flag 1 if stop. others if run.
     */
    void setStopFlag(int flag);

   /**
     * get downloaded/uploaded size.
     * 
     * @param h hash to be checked.
     * @return downloaded/uploaded file size
     */
    size_t getProgress(unsigned char* hash);
};
#endif
