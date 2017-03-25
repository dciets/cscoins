import time
import posix_ipc
import subprocess
import random

global hasStarted
hasStarted = False

# / Create the message queue, unlink them, and reopen them to make
# / sure they do not contain anything already. (Useful in case the
# / application crashed and was unable to close the queues.
serverQueue = posix_ipc.MessageQueue("/CS2017_SERVER_QNAME", posix_ipc.O_CREAT)
clientQueue = posix_ipc.MessageQueue("/CS2017_CLIENT_QNAME", posix_ipc.O_CREAT)
serverQueue.close()
clientQueue.close()
serverQueue.unlink()
clientQueue.unlink()
serverQueue = posix_ipc.MessageQueue("/CS2017_SERVER_QNAME", posix_ipc.O_CREAT)
clientQueue = posix_ipc.MessageQueue("/CS2017_CLIENT_QNAME", posix_ipc.O_CREAT)
serverQueue.block = False
clientQueue.block = False

def recv_client(*unused):
    global hasStarted
    clientQueue.request_notification((recv_client, None))
    msg = ""
    try:
        while True:
            msg = clientQueue.receive()[0]
    except:
        pass
    hasStarted = True
    print "CLIENT: " + msg + "\n"

# set the call back function for when the client sends a message
clientQueue.request_notification((recv_client, None))

random.seed()

proc = subprocess.Popen(["./client"])

#time.sleep(5)

try:
    serverQueue.send("0 sorted_list 0000000000000000000000000000000000000000000000000000000000000000 94f9 20")
    s = random.randint(2, 10)
    print "waiting " + str(s) + "s...\n"
    time.sleep(s)
    serverQueue.send("2 sorted_list 0000000000000000000000000000000000000000000000000000000000000000 94f9 800")
    s = random.randint(2, 10)
    print "waiting " + str(s) + "s...\n"
    time.sleep(s)
    serverQueue.send("3 sorted_list 0000000000000000000000000000000000000000000000000000000000000000 94f9 600")
    s = random.randint(2, 10)
    print "waiting " + str(s) + "s...\n"
    time.sleep(s)
    serverQueue.send("4 sorted_list 0000000000000000000000000000000000000000000000000000000000000000 94f9 400")
    s = random.randint(2, 10)
    print "waiting " + str(s) + "s...\n"
    time.sleep(s)
except Exception as e:
    print str(e)

serverQueue.close()
clientQueue.close()
serverQueue.unlink()
clientQueue.unlink()
proc.kill()


