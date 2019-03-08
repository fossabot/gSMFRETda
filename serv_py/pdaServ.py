from __future__ import print_function
from protobuf import args_pb2
from nanomsg import Socket, REP
import time
def paramsServ(port):
    pb_n=args_pb2.p_n()
    pb_n.s_n=3
    s1 = Socket(REP)
    print('tcp://*:'+port)
    s1.bind('tcp://*:'+port)
    time.sleep(0.5)
    while(True):
        # s1.send("connected")
        recvstr=s1.recv()
        pb_cap=args_pb2.p_cap()
        pb_cap.ParseFromString(recvstr)
        print(pb_cap.cap)
        s1.send(pb_n.SerializeToString())
        okstr=s1.recv()
        print(okstr)
        pb_ga=args_pb2.p_ga()
        pb_ga.start=0
        pb_ga.stop=pb_cap.cap
        for i in range(pb_n.s_n*pb_n.s_n):
            pb_ga.params.append(i)    
        s1.send(pb_ga.SerializeToString())
    
    s1.close()

if __name__ == '__main__':
    import sys,getopt
    dbname=''
    savefn=''
    state=2
    pick='out.pickle'
    bins=70
    maxiter=1000
    port='7777'
    try:  
        opts, args = getopt.getopt(sys.argv[1:], "l:i:s:o:b:m:p:", ["log=", "dat=","state=","pickle=","bin=","maxiter=","port="])  
        for o, v in opts: 
            if o in ("-l", "--log"):
                savefn = v
            if o in ("-b", "--bin"):
                bins = int(v.strip())                
            if o in ("-p", "--port"):
                port = v
            if o in ("-m", "--maxiter"):
                maxiter = int(v.strip())                                
            if o in ("-i", "--dat"):
                dbname=v
            if o in ("-o", "--pickle"):
                pick=v                
            if o in ("-s", "--state"):
                state = int(v.strip())
                # print(state)

    except getopt.GetoptError:  
        print("getopt error!")    
        # usage()    
        sys.exit(1)
    paramsServ(port)