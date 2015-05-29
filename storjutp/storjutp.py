# -*- coding: utf-8 -*-

# Copyright (c) 2015, Shinya Yagyu
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import types
import threading
import logging

import utpbinder
# import telehashbinder #for creating document

log_fmt = '%(filename)s:%(lineno)d %(funcName)s() %(message)s'
logging.basicConfig(level=logging.DEBUG, format=log_fmt)


class Storjutp(object):

    """
    Concrete Messaging layer for Storj Platform in Telehash.
    Everything in telehash-C is not thread safe. So run function after
    stop a thread, and run a thread again in all functions.
    """

    """ description about this messaging implementation which is
    used in sublcass.
    """

    def __init__(self, port=0):
        """
        init

        :param ChannelHandler broadcast_handler: broadcast handler.
        :param keywords keywords: 'port=int' to be listened packets.
        if 0, port number
        is seletcted randomly.
        """
        self.cobj = utpbinder.init(port)
        self.start_thread()

    def start_thread(self):
        """star to receive netowrk packets in a thread. """

        utpbinder.set_stopflag(self.cobj, 0)
        self.thread = threading.Thread(
            target=lambda: utpbinder.start(self.cobj))
        self.thread.setDaemon(True)
        self.thread.start()

    def regist_hash(self, hash, handler, directory='.'):
        """
        open a channel with a handler.

        :param str location: json str where you want to open a channel.
        :param str name: channel name that you want to open .
        :param ChannelHandler handler: channel handler.
        """
        utpbinder.set_stopflag(self.cobj, 1)
        self.thread.join()
        r = utpbinder.regist_hash(self.cobj, hash, handler, directory)
        self.start_thread()
        return r

    def stop_hash(self, hash):
        """
        open a channel with a handler.

        :param str location: json str where you want to open a channel.
        :param str name: channel name that you want to open .
        :param ChannelHandler handler: channel handler.
        """
        utpbinder.set_stopflag(self.cobj, 1)
        self.thread.join()
        utpbinder.stop_hash(self.cobj, hash)
        self.start_thread()

    def get_progress(self, hash):
        """
        open a channel with a handler.

        :param str location: json str where you want to open a channel.
        :param str name: channel name that you want to open .
        :param ChannelHandler handler: channel handler.
        """
        utpbinder.set_stopflag(self.cobj, 1)
        self.thread.join()
        size = utpbinder.get_progress(self.cobj, hash)
        self.start_thread()
        return size

    def send_file(self, dest, port, fname, hash, handler):
        """
        send a broadcast request to broadcaster.
        After calling this method, broadcast messages will be send continually.

        :param str location: json str where you want to request a broadcast.
        :param  int add: if 0, request to not to  broadcast. request to
                          broaadcast if others.
        """
        utpbinder.set_stopflag(self.cobj, 1)
        self.thread.join()
        r = utpbinder.send_file(self.cobj, dest, port, fname, hash, handler)
        self.start_thread()
        return r

    def get_serverport(self):
        """
        send a broadcast request to broadcaster.
        After calling this method, broadcast messages will be send continually.

        :param str location: json str where you want to request a broadcast.
        :param  int add: if 0, request to not to  broadcast. request to
                          broaadcast if others.
        """
        return utpbinder.get_serverport(self.cobj)

    def finalize(self):
        """
         destructor. stop a thread and call telehashbinder's finalization.
        """
        utpbinder.set_stopflag(self.cobj, 1)
        self.thread.join()
        utpbinder.finalize(self.cobj)
