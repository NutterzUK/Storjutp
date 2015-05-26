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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <algorithm>
#include <array>


#include "utp.h"
#include "Storjutp.h"

#define TIMEOUT 500
#define BUFSIZE 65536


#define LOG(fmt, ...) log(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
void *log(const char *file, int line, const char *function,
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


string byte2string(unsigned char *bytes, string& result){
    int i=0;
    for(i=0;i<32;i++){
        char s[3];
        sprintf(s, "%02X",bytes[i]);
        result += s;
    }
    return result;
}

uint64 callback_on_read(utp_callback_arguments *a){
    FileInfo *fi= (FileInfo *)utp_get_userdata(a->socket);
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);

   	size_t left = a->len;
    const byte *p = a->buf;

   	LOG("read on fd=%d len=%d" , sutp->fd,left);

    if(!fi && left > 32){
        list<FileInfo*>::iterator itr;
        for(itr = sutp->fileInfos.begin();
            itr != sutp->fileInfos.end();itr++){
            int j=0;
            bool match = true;
            for(j=0;j<32;j++){
                if((*itr)->hash[j] != p[j]){
                    match=false;
                    break;
                }
            }
            if(match) break;
        }
        if(itr == sutp->fileInfos.end()){
         	LOG("rejected");
            return 0;
        }
      	LOG("newly accepted");
        fi = *itr;
        fi->socket = a->socket;
        utp_set_userdata(a->socket, fi);
	    left -= 32;
        p+=32;
   	}
        
    if(fi){
        size_t len=0;
    	for(;left; left -=len, p+=len) {
	    	len = fwrite(p, 1, left, fi->fp);
    	}
       	LOG("wrote len=%d to file", len);
    }
    if(left==0){
    	utp_read_drained(a->socket);
    }
	return 0;
}

uint64 callback_on_error(utp_callback_arguments *a){
	LOG("Error: %s", utp_error_code_names[a->error_code]);
    FileInfo *fi = (FileInfo *)utp_get_userdata(a->socket);
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);

    if(fi->handler){
        fi->handler->on_finish(fi->hash,
                               utp_error_code_names[a->error_code]);
    }
    sutp->deleteFileInfo(fi);
    utp_close(a->socket);
    return 0;
}

uint64 callback_on_accept(utp_callback_arguments *a){
   	LOG("on_accept");
    //from server to client
    // do return 0, or not accepted.
	return 0;
}

void sendFile(utp_socket *s, FileInfo *fi, Storjutp *sutp){
    size_t len = 0, len_u = 0;
    do {
        unsigned char buf[BUFSIZE];
        len = fread(buf, 1, BUFSIZE, fi->fp);
        if(len > 0){
            len_u = utp_write(s, buf, len);
          	LOG("utp_write len=%d", len_u);
        }
        if(len_u < len){
            fseek(fi->fp, len_u - len, SEEK_CUR);
        }
    }while(len == BUFSIZE && len_u > 0);

    if(feof(fi->fp)){
        fi->handler->on_finish(fi->hash, NULL);
        utp_close(s);
        sutp->deleteFileInfo(fi);
    }
}

uint64 callback_on_state_change(utp_callback_arguments *a){
    FileInfo *fi = (FileInfo *)utp_get_userdata(a->socket);
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);

	switch (a->state) {
		case UTP_STATE_CONNECT:
        {
         	LOG("connected");
            utp_write(a->socket, fi->hash, 32);
	        sendFile(a->socket, fi, sutp);
            break;
        }
		case UTP_STATE_WRITABLE:
        {
         	LOG("writable");
        //from client to server
	        sendFile(a->socket, fi, sutp);
			break;
        }
		case UTP_STATE_EOF:
        {
        	LOG("state_eof");
            if(fi->handler){
                fi->handler->on_finish(fi->hash, NULL);
            }
            sutp->deleteFileInfo(fi);
            utp_close(a->socket);
            
			break;
         }
		case UTP_STATE_DESTROYING:
        {
          	LOG("destyoed");
            sutp->setStopFlag(1);
			break;
        }
    }

	return 0;
}


uint64 callback_sendto(utp_callback_arguments *a){
    Storjutp *sutp = (Storjutp *) utp_context_get_userdata(a->context);
	size_t l = sendto(sutp->fd, a->buf, a->len, 0,
                       a->address, a->address_len);
   	LOG("really sended len=%d/%d from fd=%d", l, a->len, sutp->fd);
	return 0;
}



struct addrinfo* getAddrInfo(string addr, int port){
    struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
    
    int error = 0;
    char p[10];
    sprintf(p, "%d", port);
    if ((error = getaddrinfo(addr.c_str(), p, &hints, &res)))
        return NULL;
	
    return res;
}

void Storjutp::deleteFileInfo(FileInfo *fi){
    fileInfos.remove(fi);
    if(fi){
        fclose(fi->fp);
        delete fi;
    }
}

Storjutp::Storjutp(int port) {
    ctx = utp_init(2);
  	utp_set_callback(ctx, UTP_ON_READ, &callback_on_read);
  	utp_set_callback(ctx, UTP_ON_ACCEPT, &callback_on_accept);
    utp_set_callback(ctx, UTP_ON_ERROR, &callback_on_error);
    utp_set_callback(ctx, UTP_ON_STATE_CHANGE,&callback_on_state_change);
  	utp_set_callback(ctx, UTP_SENDTO, &callback_sendto);

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    utp_context_set_userdata(ctx, this);
    for(server_port = port; server_port < 65535; server_port++){
        struct addrinfo *res = getAddrInfo("0.0.0.0", server_port);
	    if(res  && !bind(fd, res->ai_addr, res->ai_addrlen)){
            freeaddrinfo(res);
            break;
        }
     	freeaddrinfo(res);
    }
    stopFlag = 0;
}


void Storjutp::registHash(unsigned char* hash, Handler *handler){
    FileInfo *fi = new FileInfo;
    memcpy(fi->hash, hash, 32);
    string fname;
    byte2string(hash, fname);
    fi->fp = fopen(fname.c_str(), "wb");
    fi->handler = handler;
    fileInfos.push_back(fi);
    LOG("regist %s at fd=%d",fname.c_str(), fd);
}

void Storjutp::unregistHash(unsigned char* hash){
    list<FileInfo*>::iterator itr;
    for( itr = fileInfos.begin(); itr != fileInfos.end(); itr++ ){
        int j=0;
        bool match=true;
        for(j=0;j<32;j++){
            if((*itr)->hash[j] != hash[j]) match=false;
        }
        if(match){
            deleteFileInfo(*itr);
            fileInfos.erase(itr);
        }
	}
}


int Storjutp::sendFile(string dest, int port, string fname, 
                             unsigned char* hash, Handler *handler){
    FILE *fp = fopen(fname.c_str(), "rb");
    if(!fp) return -1;
    FileInfo *f= new FileInfo;
    f->fp = fp;
    LOG("sending %s at fd=%d",fname.c_str(), fd);
    f->socket = utp_create_socket(ctx);
    struct addrinfo *res = getAddrInfo(dest, port);
    utp_connect(f->socket, res->ai_addr, res->ai_addrlen);
  	freeaddrinfo(res);
    utp_set_userdata(f->socket, f);
    f->isSending = true;
    memcpy(f->hash, hash, 32);
    f->handler = handler;
    fileInfos.push_back(f);
    return 0;
}

void Storjutp::setStopFlag(int flag){
    stopFlag=flag;
}

void Storjutp::start(){
    LOG("running fd=%d", fd);
    unsigned char socket_data[BUFSIZE];
   	struct sockaddr_in src_addr;
   	socklen_t addrlen = sizeof(src_addr);
    struct pollfd fds[1];
  
    while(!stopFlag) {
        fds[0].fd = fd;
        fds[0].events = POLLIN | POLLERR;
        poll(fds, 1, TIMEOUT);
        if (fds[0].revents & POLLIN) {
            int len = 0;
            do{
                len = recvfrom(fd, socket_data, BUFSIZE, MSG_DONTWAIT, 
                               (struct sockaddr *)&src_addr, &addrlen);
                LOG("really recved len=%d at fd=%d",len,fd);
                if (len < 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        utp_issue_deferred_acks(ctx);
                    }
                }else{
                    if(!utp_process_udp(ctx, socket_data, len, 
                                    (struct sockaddr *)&src_addr, addrlen)){
                        LOG("invalid packet fd=%d",fd);
                    }
                }
            }while(len>0);
        }
      	utp_check_timeouts(ctx);
    }
}        
    
Storjutp::~Storjutp() {
    list<FileInfo*>::iterator itr;
    for( itr = fileInfos.begin(); itr != fileInfos.end(); itr++ ){
        deleteFileInfo(*itr);
	}
 	utp_destroy(ctx);
}
