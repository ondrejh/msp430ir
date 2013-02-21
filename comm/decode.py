#! /usr/bin/python

from serial import Serial

portname='COM6'
#portname='/dev/ttyACM0'
hellostring='M430IR_V001'

codestoread=5
codescnt=0
codes = []
lines = []

def decode_line(line):
	''' Module should decode ir code data redeived on serial port
	(with M430IR connected on it).
	Returns values if success or error string if fails'''
	
	try:
		linesplit = line.split(':')
		linelen = int(linesplit[0],base=16)
		try:
			linesplit = linesplit[1].split(';')
			if len(linesplit)!=linelen:
				return('Number of entries doesn\'t fit') #error 3
			tim=[]
			for entid in range(len(linesplit)):
				n=int(linesplit[entid],base=16)
				tim=tim+[n]
			return(linelen,tim) #no error (just return values)

		except:
			return('Can\'t read entries') #error 2

	except:
		return('Can\'t find line length') #error 1

def expand_to_plot(decoded_line):
	''' expand value/time lists to plot it '''

	try:
		if decoded_line[0]==len(decoded_line[1]):
			tabs=0
			vabs=0
			tout=[-1000,0]
			vout=[vabs,vabs]
			for listid in range(decoded_line[0]):
				tout+=[tabs]
				tabs=decoded_line[1][listid]
				tout+=[tabs]
				vabs^=0x01
				vout+=[vabs,vabs]
			vabs^=0x01
			tout+=[tabs,tabs+1000]
			vout+=[vabs,vabs]
			return(vout,tout) #return values
		else:
			return('Lists must be the same length longer than 0') #error
		
	except:
		return('Wrong decoded line format') #error



if __name__ == "__main__":
	''' main module: open port, test if M430IR connected, read codes '''
	
	with Serial(portname,9600,timeout=0.5) as port:
		port.write('?'.encode('ascii'))
		port.flush()
		answ = port.readlines()
		if len(answ)>0:
			answ=answ[len(answ)-1].strip().decode('ascii')
			print('Init module answer: {}'.format(answ))

			while (True):
				line = port.readline()
				if len(line)>0:
					line=line.strip().decode('ascii')
					#print(line)
					lindec=decode_line(line)
					print(lindec)
					
					if type(lindec[0])==int:
						codes+=[lindec]
						lines+=[expand_to_plot(lindec)]
						#print(expand_to_plot(lindec))
						codescnt+=1
						if codescnt>=codestoread:
							break

			port.close()

		else:
			print('Init module answer: No reponse .. break')

	from pylab import *

	figure(1)
	for line in lines:
		plot(line[1],line[0])
	grid('on')
	axis([None,None,-0.5,1.5])
	show()
