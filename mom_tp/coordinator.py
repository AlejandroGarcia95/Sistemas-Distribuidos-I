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
	def __init__(self):
		# shmTable has shms stored as {name: value}
		# semTable has sems stored as {name: (value, [process list])}
		self.semTable = {}
		self.shmTable = {}
		self.mom = Mom()
		self.mom.subscribe(COORDINATOR_TOPIC)

	def _shmInit(self, name, value):
		if name in self.shmTable:
			return False
		self.shmTable[name] = value
		return True

	def _shmDestroy(self, name):
		if not name in self.shmTable:
			return False
		self.shmTable.pop(name)
		return True

	def _shmRead(self, name):
		if name in self.shmTable:
			return self.shmTable[name]
		return None

	def _shmWrite(self, name, value):
		if name in self.shmTable:
			self.shmTable[name] = value
			return True
		return False

	def _semInit(self, name, value):
		if name in self.semTable:
			return False
		if value < 0:
			return False
		self.semTable[name] = (value, [])
		return True

	def _semWait(self, name, processId):
		if name in self.semTable:
			value, pList = self.semTable[name]
			if value > 0:
				self.semTable[name] = (value - 1, pList)
				return True
			else:	# Have to block process, hence dont answer
				pList.append(processId)
				self.semTable[name] = (0, pList)
				return None
		return False

	def _semSignal(self, name):
		if name in self.semTable:
			value, pList = self.semTable[name]
			if len(pList) > 0:
				if value > 0:
					print "SOMETHING WENT TERRIBLY WRONG: SEM HASNT PROPERLY WAIT"
				nextProc = pList.pop(0)
				# Wakeup nextProc
				processTopic = "Museum/" + nextProc
				self.mom.publish(processTopic, "Coord:1")
			else:
				value += 1
			self.semTable[name] = (value, pList)
			return True
		return False
		
	def _semDestroy(self, name):
		if name in self.semTable:
			value, pList = self.semTable[name]
			# Wakeup all processes
			for proc in pList:
				processTopic = "Museum/" + proc
				self.mom.publish(processTopic, "Coord:0")
			self.semTable.pop(name)
			return True
		return False		

	def fetchNextOrder(self):
		return self.mom.receive()
	
	def _processOrder(self, order):
		lOrder = order.split()
		if len(lOrder) < 3:
			return "0"
		processId = lOrder[0].split(":")[0]
		shrdRsc = lOrder[0].split(":")[1].upper()
		action = lOrder[1].upper()
		name = lOrder[2].rstrip('\x00').upper()
		if shrdRsc == "SHM":
			# shm_init
			if action == "INIT":
				return self._shmInit(name, int(lOrder[3]))
			# shm_read
			elif action == "READ":
				r = self._shmRead(name)
				if r is not None:
					return str(r)
				else:
					return "0"	# Be careful not to confuse with 0 value
			# shm_write
			elif action == "WRITE":
				return self._shmWrite(name, int(lOrder[3]))
			# shm_destoy
			elif action == "DESTROY":
				return self._shmDestroy(name)
			# Wrong shm action		
			else:
				return "0"
		
		elif shrdRsc == "SEM":
			# sem_init
			if action == "INIT":
				return self._semInit(name, int(lOrder[3]))
			# sem_signal
			elif action == "SIGNAL":
				return self._semSignal(name)
			# sem_wait
			elif action == "WAIT":
				return self._semWait(name, processId)
			# sem_write
			elif action == "DESTROY":
				return self._semDestroy(name)
			# Wrong sem action
			else:
				return "0"
		# Wrong shared resource word
		else:
			return "0"
		
	
	def executeOrder(self, order):
		if len(order) <= 5:
			return
		r = self._processOrder(order)
		processTopic = "Museum/" + order.split()[0].split(":")[0]
		if r is None:
			return
		elif r == True:
			self.mom.publish(processTopic, "Coord:1")
		elif r == False:
			self.mom.publish(processTopic, "Coord:0")
		else:
			self.mom.publish(processTopic, "Coord:" + str(r))
		
	def printStatus(self):
		print "STATUS"
		print("SHMs: " + str(self.shmTable))
		print("SEMs: " + str(self.semTable))
		
		
# Main

cnator = Coordinator()
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
