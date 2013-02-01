#! /usr/bin/python3

from serial import Serial

portname='COM6'
hellostring='M430IR_V001'

with Serial(portname,9600,timeout=0.2) as port:
    port.write('?'.encode('ascii'))
    answ = port.readlines()
    if len(answ)>0:
        answ=answ[len(answ)-1].strip().decode('ascii')
        print('Init module answer: {}'.format(answ))

        port.timeout=None
        while (True):
            line = port.readline()
            print(line.strip().decode('ascii'))
        
    else:
        print('Init module answer: No reponse .. break')
