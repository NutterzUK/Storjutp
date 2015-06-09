"""
  uTP file transfer library binder implmeneted in C++.
"""


def init(port):
    """
    init.

    :param int port: port number to be listened packets. if 0, port number is.
    """
    pass


def regist_hash(cobj, hash, handler, dir):
    """
    register acceptable file hash.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param bytes hash: acceputable file hash.
    :param method handler: Handler called when finish receiving a file.
    handler method must have hash(bytes) and errormessage(str)
    arguments.
    :param str dir:  directory where file will be saved.
    :return 0 if success
    """
    pass


def get_serverport(cobj):
    """
    get listening server port.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :return:  port number int.
    """
    pass


def stop_hash(cobj, hash):
    """
    unregister a hash and stop sending/downloading  file.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param bytes hash: acceputable file hash to be unregistered.
    """
    pass


def start(cobj):
    """
    star to receive/send netowrk packets in the current thread..

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    """
    pass


def set_stopflag(cobj, stop):
    """
    set stopFlag that stop/continue to receive\send network packets loop..

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param int flag: 1 if to stop. others if to run.
    """
    pass


def finalize(cobj):
    """
    destructor.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    """
    pass


def send_file(cobj, dest, port, fname, hash, handler):
    """
    start to send a file.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param int port: destination port to be sent.
    :param str fname: file name to be sent.
    :param bytes hash: file hash.
    :param method handler: Handler called when finishing uploading.
     """
    pass


def get_progress(cobj, hash):
    """
    get downloaded/uploaded size.

    :param Object cobj: pointer of StorjTelehash instnace returned by init()
    :param bytes hash: file hash to be checked.
    :return: downloaded/uploaded file size
     """
    pass

