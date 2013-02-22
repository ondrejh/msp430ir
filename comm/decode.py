#! /usr/bin/python

from serial import Serial

#codes visualisation uses matplotlib (if not install set False)
showcodes = False
if showcodes:
	from pylab import *


portname='COM6'
#portname='/dev/ttyACM0'
hellostring='M430IR_V001'


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


def GetCode(port,codestoread):
	''' get code function
	opens port, tests if msp430ir receiver connected, read N codes and
	return [codes,lines]
		codes ... raw codes [length,[tim1,tim2,..,timN]]
		lines ... the codes converted to plotable form'''

	codes = []
	lines = []
	codescnt = 0
	
	with Serial(portname,9600,timeout=0.5) as port:
		port.write('?'.encode('ascii'))
		port.flush()
		answ = port.readlines()
		if len(answ)>0:
			answ=answ[len(answ)-1].strip().decode('ascii')
			print('Init module answer: {}'.format(answ))

			print('Press ir remote button {} times'.format(codestoread-codescnt))
			while (True):
				line = port.readline()
				if len(line)>0:
					line=line.strip().decode('ascii')
					#print(line)
					lindec=decode_line(line)
					#print(lindec)
					
					if type(lindec[0])==int:
						codes+=[lindec]
						lines+=[expand_to_plot(lindec)]
						#print(expand_to_plot(lindec))
						codescnt+=1
						if codescnt>=codestoread:
							break
						else:
							print('Samples to read {}'.format(codestoread-codescnt))

			port.close()

		else:
			print('Init module answer: No reponse !!!')

	return ([codes,lines])


def PlotCode(lines):
	''' plot code with matplotlib '''

	figure(1)
	for line in lines:
		plot(line[1],line[0])
	grid('on')
	axis([None,None,-0.5,1.5])
	show()


def GetAvgCode(codes):
	''' get codes list and return average code with differences
	return [avcode,avdiff]
	if codes not the same length avcode is empty '''

	avlen = codes[0][0]
	codescnt = len(codes)
	for codeid in range(codescnt):
		if avlen!=codes[codeid][0]:
			avlen=None
		if avlen!=None:
			avcode=[]
			avmax=[]
			avmin=[]
			avdiff=[]
			for codeid in range(codescnt):
				for itemid in range(len(codes[0][1])):
					val = codes[codeid][1][itemid]
					if codeid>0:
						avcode[itemid]+=val
						if avmax[itemid]<val:
							avmax[itemid]=val
						if avmin[itemid]>val:
							avmin[itemid]=val
					else:
						avcode+=[val]
						avmax+=[val]
						avmin+=[val]
						avdiff+=[0]
								
			for itemid in range(avlen):
				avcode[itemid]=round(avcode[itemid]/codescnt)
				avdiff[itemid]=avmax[itemid]-avmin[itemid]
				
			return([avcode,avdiff])
		
		else:
			print('Codes doesn\'t have the same length')


def GenerateIRCodeFile(codenames,codes):
	''' this should generate c file containing ir codes, its lengths and names '''

	f = open('ircodes.h',mode='w')

	f.write('/**\n * ircodes.h automatically generated file of ir codes\n *\n **/\n\n')
	f.write('#ifndef __IRCODES_H__\n#define __IRCODES_H__\n\n')
	f.write('#include <inttypes.h>\n\n')

	ncodes = len(codenames)
	f.write('#define IRCODES {}\n\n'.format(ncodes))

	ircodes_data_len=0
	for codeid in range(ncodes):
		ircodes_data_len+=1+len(codes[codeid])

	if ncodes>0:
		f.write('const char *ircode_name[IRCODES] = {\n')
		for codeid in range(len(codenames)):
			f.write('    "{}"{}'.format(codenames[codeid],
						 '};\n\n' if codeid==(ncodes-1) else ',\n'))
		f.write('const uint16_t ircode_data[{}] = '.format(ircodes_data_len)+'{\n')
		for codeid in range(len(codenames)):
			f.write('    {}, // code {}: {}\n    '.format(len(codes[codeid]),
								      codeid,
								      codenames[codeid]))
			for itemid in range(len(codes[codeid])):
				f.write('{}{}'.format(codes[codeid][itemid],
						      ',' if itemid!=(len(codes[codeid])-1) else (',\n' if codeid!=(len(codes)-1) else '};\n')))

	f.write('\n#endif\n')
	

if __name__ == "__main__":
	''' main module: open port, test if M430IR connected, read codes '''

	mycodes = []
	mycodecnt = 0
	mycodenames = []
	
	codestoread = 0
	while(True):
		codestoread = input('How many samples to read (per one code): ')
		try:
			codestoread = int(codestoread)
			if (codestoread>0) and (codestoread<21):
				break
			raise Exception
		except:
			print('It must be whole number between 1 and 20')
	
	while(True):

		name = ''
		
		while(True):
			name = input('Please enter name of the code {}: '.format(mycodecnt))
			try:
				name = str(name)
				if (len(name)>0) and (len(name)<9):
					break
				raise Exception
			except:
				print('It must be string of 1 to 8 letters')
	
		[codes,lines] = GetCode(portname,codestoread)

		if len(lines)>0:
			if showcodes:
				PlotCode(lines)
			[avcode,avdiff] = GetAvgCode(codes)
			if avcode!=[]:
				mycodes += [avcode]
				mycodenames += [name]
				mycodecnt+=1
				print('Code stored OK')
				print(avcode)

		cont = False
		answ = input('Would you like to continue ? (Y/N)[N]: ')
		if (answ=='Y') or (answ=='y'):
			cont=True

		if cont!=True:
			break

	#print(mycodenames)
	#print(mycodes)

	print('Generating ircodes.h file')
	GenerateIRCodeFile(mycodenames,mycodes)

