#! /usr/bin/python

from serial import Serial

#portname='COM6'
portname='/dev/ttyACM0'
hellostring='M430IR_V001'

def decode_line(line):
	''' Module should decode ir code data redeived on serial port (with M430IR
	connected on it). It returns x,y plot coordinates if success or error string
	if fails'''
	
	try:
		linesplit = line.split(':')
		linelen = int(linesplit[0],base=16)
		try:
			linesplit = linesplit[1].split(';')
			if len(linesplit)!=linelen:
				return('Number of entries doesn\'t fit') #error 3
			tim=0
			lev=1
			if int(linesplit[0],base=16)>=0x8000:
				lev=0
			x=[-1000,0]
			y=[lev,lev]
			for entid in range(len(linesplit)):
				n=int(linesplit[entid],base=16)
				if n>=0x8000:
					n=n-0x8000
					lev=1
				else:
					lev=0
				y=y+[lev,lev]
				x=x+[tim]
				tim=tim+n
				x=x+[tim]
			if lev==0:
				lev=1
			else:
				lev=0
			y=y+[lev,lev]
			x=x+[tim,tim+1000]
			return(x,y) #no error (just return values)

		except:
			return('Can\'t read entries') #error 2

	except:
		return('Can\'t find line length') #error 1


with Serial(portname,9600,timeout=0.2) as port:
	port.write('?'.encode('ascii'))
	port.flush()
	answ = port.readlines()
	if len(answ)>0:
		answ=answ[len(answ)-1].strip().decode('ascii')
		print('Init module answer: {}'.format(answ))

		#port.timeout=5.0
		while (True):
			line = port.readline()
			if len(line)>0:
				line=line.strip().decode('ascii')
				print(line)
				print(decode_line(line))

	else:
		print('Init module answer: No reponse .. break')

