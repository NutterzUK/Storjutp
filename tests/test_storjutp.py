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


import pytest
import time
import logging
import filecmp
import os

from storjutp.storjutp import Storjutp

log_fmt = '%(filename)s:%(lineno)d %(funcName)s() %(message)s'
logging.basicConfig(level=logging.DEBUG, format=log_fmt)


class TestStorjutp(object):

    def handler_sender(self, hash, errorMessage):
        logging.debug("sender finished")
        if errorMessage is not None:
            logging.debug(" errorMessage:" + errorMessage)

        assert errorMessage is None
        self.sender_finish = True

    def handler_receiver(self, hash, errorMessage):
        logging.debug("receiver finished")
        if errorMessage is not None:
            logging.debug(" errorMessage:" + errorMessage)

        assert errorMessage is None
        self.receiver_finish = True

    def test_storjutp(self):
        self.sender_finish = False
        self.receiver_finish = False
        fname = \
            'C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7'
        dummy = bytearray([i for i in range(200, 232)])

        if os.path.exists(fname):
            os.remove(fname)

        s1 = Storjutp(12345)
        s2 = Storjutp()
        assert s1.get_serverport() == 12345

        s1.regist_hash(dummy, self.handler_receiver)
        r = s1.regist_hash(dummy, self.handler_receiver)
        assert r
        s1.stop_hash(dummy)
        r = s1.regist_hash(dummy, self.handler_receiver)
        assert not r

        s2.send_file('127.0.0.1', 12345, 'tests/rand.dat', dummy,
                     self.handler_sender)
        time.sleep(1)
        size1 = s1.get_progress(dummy)
        size2 = s1.get_progress(dummy)
        logging.debug('reciever progress : %d' % size1)
        logging.debug('reciever progress : %d' % size2)
        assert 0 < size1 and size1 < 25424239
        assert 0 < size2 and size2 < 25424239
        time.sleep(60)
        assert filecmp.cmp(fname, "tests/rand.dat")

        assert self.receiver_finish
        assert self.sender_finish

        s1.finalize()
        s2.finalize()
