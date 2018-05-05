#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *

# ------ Python Coodinator: The boss of SHM and SEM ------

# Format of received messages:

# "processId:<SHM/SEM> <ACTION> <NAME> <VALUE> <REQ NUMBER>"
# Examples: "1234:SEM INIT SOMESEM 3", "1234:SHM READ SOMESHM", "1234:SEM WAIT SOMESEM"

# Feasible responses:

# "Coord:<INT VALUE>" (example: "8", for shmRead)
# "Coord:<1/0>" (i.e. success, failure)

COORDINATOR_TOPIC = "Museum/Coordinator/Coordinator"

class Coordinator:
	def __init__(self, mom):
		# shmTable has shms stored as {name: value}
		# semTable has sems stored as {name: (value, [process list])}
		self.semTable = {}
		self.shmTable = {}
		self.mom = mom

	def _shmInit(self, name, value, processTopic):
		if name in self.shmTable:
			return [ (processTopic, False) ]
		self.shmTable[name] = value
		return [ (processTopic, True) ]

	def _shmDestroy(self, name, processTopic):
		if not name in self.shmTable:
			return [ (processTopic, False) ]
		self.shmTable.pop(name)
		return [ (processTopic, True) ]

	def _shmRead(self, name, processTopic):
		if name in self.shmTable:
			return [ (processTopic, str(self.shmTable[name])) ]
		return [ (processTopic, None) ]

	def _shmWrite(self, name, value, processTopic):
		if name in self.shmTable:
			self.shmTable[name] = value
			return [ (processTopic, True) ]
		return [ (processTopic, False) ]

	def _semInit(self, name, value, processTopic):
		if name in self.semTable:
			return [ (processTopic, False) ]
		if value < 0:
			return [ (processTopic, False) ]
		self.semTable[name] = (value, [])
		return [ (processTopic, True) ]

	def _semWait(self, name, processId, processTopic):
		if name in self.semTable:
			value, pList = self.semTable[name]
			if value > 0:
				self.semTable[name] = (value - 1, pList)
				return [ (processTopic, True) ]
			else:	# Have to block process, hence dont answer
				pList.append(processId)
				self.semTable[name] = (0, pList)
				return [ (processTopic, None) ]
		return [ (processTopic, False) ]

	def _semSignal(self, name, processTopic):
		responses = []
		if name in self.semTable:
			value, pList = self.semTable[name]
			if len(pList) > 0:
				if value > 0:
					print "SOMETHING WENT TERRIBLY WRONG: SEM HASNT PROPERLY WAIT"
				nextProc = pList.pop(0)
				# Wakeup nextProc
				nextProcTopic = "Museum/" + nextProc
				responses.append( (nextProcTopic, True) )
			else:
				value += 1
			self.semTable[name] = (value, pList)
			responses.append( (processTopic, True) )
			return responses
		return [ (processTopic, False) ]
		
	def _semDestroy(self, name, processTopic):
		responses = []
		if name in self.semTable:
			value, pList = self.semTable[name]
			# Wakeup all processes
			for proc in pList:
				procTopic = "Museum/" + proc
				responses.append( (procTopic, False) )
			self.semTable.pop(name)
			responses.append( (processTopic, True) )
			return responses
		return [ (processTopic, False) ]		

	
	def _processOrder(self, order, processTopic):
		lOrder = order.split()
		if len(lOrder) < 3:
			return [ (processTopic, "0") ]
		processId = lOrder[0].split(":")[0]
		shrdRsc = lOrder[0].split(":")[1].upper()
		action = lOrder[1].upper()
		name = lOrder[2].rstrip('\x00').upper()
		if shrdRsc == "SHM":
			# shm_init
			if action == "INIT":
				return self._shmInit(name, int(lOrder[3]), processTopic)
			# shm_read
			elif action == "READ":
				resp = self._shmRead(name, processTopic)
				if resp[0][1] is not None:
					return resp
				else:
					return [ (processTopic, "0") ]	# Be careful not to confuse with 0 value
			# shm_write
			elif action == "WRITE":
				return self._shmWrite(name, int(lOrder[3]), processTopic)
			# shm_destoy
			elif action == "DESTROY":
				return self._shmDestroy(name, processTopic)
			# Wrong shm action		
			else:
				return [ (processTopic, "0") ]
		
		elif shrdRsc == "SEM":
			# sem_init
			if action == "INIT":
				return self._semInit(name, int(lOrder[3]), processTopic)
			# sem_signal
			elif action == "SIGNAL":
				return self._semSignal(name, processTopic)
			# sem_wait
			elif action == "WAIT":
				return self._semWait(name, processId, processTopic)
			# sem_write
			elif action == "DESTROY":
				return self._semDestroy(name, processTopic)
			# Wrong sem action
			else:
				return [ (processTopic, "0") ]
		# Wrong shared resource word
		else:
			return [ (processTopic, "0") ]
		
	def executeOrder(self, order):
		if len(order) <= 5:
			return
		processTopic = "Museum/" + order.split()[0].split(":")[0]
		return self._processOrder(order, processTopic)
		
		
	def sendResponses(self, responses):
		for r in responses:
			if r[1] is None:
				continue
			elif r[1] == True:
				print str(r) + " becomes " + str(r[0]) + " Coord:1"
				self.mom.publish(r[0], "Coord:1")
			elif r[1] == False:
				print str(r) + " becomes " + str(r[0]) + " Coord:0"
				self.mom.publish(r[0], "Coord:0")
			else:
				print str(r) + " becomes " + str(r[0]) + " " + str(r[1])
				self.mom.publish(r[0], "Coord:" + str(r[1]))
		
	def printStatus(self):
		print "STATUS"
		print("SHMs: " + str(self.shmTable))
		print("SEMs: " + str(self.semTable))
		
"""		
# Main
mom = Mom()
mom.subscribe(COORDINATOR_TOPIC)
cnator = Coordinator(mom)
_ = os.system("clear")
print "Coordinator is up!"
while(1):
	try:
		print "Awaiting order..."
		order = cnator.fetchNextOrder()
		print("Executing order: " + order)
		cnator.executeOrder(order)
		cnator.printStatus()
	except:
		print "Closing coordinator..."
		break
"""
