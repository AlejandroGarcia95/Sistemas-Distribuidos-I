#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys
from threading import *
import os
import signal
import time

# Must be called as "python new_coordinator.py <cId> <cAmount>"

REDUNDANCY = 2

# ------------------------ Topics used ------------------------

ALIVE_TOPIC = "Philo/Coordinator/Alive"
COORDINATOR_TOPIC = "Philo/Coordinator/Coordinator"
PROXIES_TOPIC = "Philo/Coordinator/Proxies"

# -------------------------- Timers --------------------------

TMR_DISCOVERY = 20.0
TMR_KEEPALIVE = 3.0
TMR_DEFUNCT = 8.5

# ------------------------- Messages -------------------------

MSG_KEEPALIVE = "ALIVE"
MSG_DISCOVERY = "DIS"
MSG_NEW_COORD_TOPIC = "NEWTOPIC "


# -------------------------------------------------------------

# ------------------------ ResourcesDB ------------------------

# Format of received messages:

# "processId:<SHM/SEM> <ACTION> <NAME> <VALUE> <REQ NUMBER>"
# Examples: "1234:SEM INIT SOMESEM 1 19", "1234:SHM READ SOMESHM 0 19", 
# "1234:SEM WAIT SOMESEM 0 19"

# Feasible responses:

# "Coord:<INT VALUE>" (example: "8", for shmRead)
# "Coord:<1/0>" (i.e. success, failure)

class ResourcesDB:
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
				nextProcTopic = "Philo/" + nextProc
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
				procTopic = "Philo/" + proc
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
		processTopic = "Philo/" + order.split()[0].split(":")[0]
		return self._processOrder(order, processTopic)
		
		
	def sendResponses(self, responses):
		for r in responses:
			if r[1] is None:
				continue
			elif r[1] == True:
				self.mom.publish(r[0], "Coord:1")
			elif r[1] == False:
				self.mom.publish(r[0], "Coord:0")
			else:
				self.mom.publish(r[0], "Coord:" + str(r[1]))
		
	def printStatus(self):
		os.write(1, "STATUS\n")
		os.write(1, "SHMs: " + str(self.shmTable) + "\n")
		os.write(1, "SEMs: " + str(self.semTable) + "\n")

# ------------------------ Coordinator ------------------------

class Coordinator:
	def __init__(self):
		self.cId = int(sys.argv[1])
		self.cAmount = int(sys.argv[2])
		self.mom = Mom()
		self.aliveCoords = {}
		self._log("Coord up! Entering discovery mode...")
		self._discoveryState()
		#for r in range(0, REDUNDANCY):
		#	self.mom.subscribe(COORDINATOR_TOPIC + str((self.cId + r) % self.cAmount))
		self.aliveTmr = None
		self.t = None
		self.db = ResourcesDB(self.mom)
		self.reqTable = {}
		

	def _updateAliveTable(self, coordId):
		if coordId > self.cAmount:
			self._log("WARNING: Received wrong coord id: " + str(coordId))
			return
		self.aliveCoords[coordId] = time.time()

	def _log(self, text):
		os.write(1, "Coord" + str(self.cId) + ": " + text + "\n")

	def _messageIsDiscovery(self, m):
		return (m[:len(MSG_DISCOVERY)] == MSG_DISCOVERY)
	
	def _messageIsKeepAlive(self, m):
		return (m[:len(MSG_KEEPALIVE)] == MSG_KEEPALIVE)
		
	def _tmrDiscovery(self):
		# If here, I've ended my discovery time
		os.kill(os.getpid(), signal.SIGINT)
	
	def _tmrKeepAlive(self):
		self._log("Sending keepalive message")
		self.mom.publish(ALIVE_TOPIC, MSG_KEEPALIVE + ":" + str(self.cId))
		self.aliveTmr = Timer(TMR_KEEPALIVE, self._tmrKeepAlive)
		self.aliveTmr.start()
	
	def _discoveryState(self):
		self.mom.publish(ALIVE_TOPIC, MSG_DISCOVERY + ":" + str(self.cId))
		# Subscribe to keepalive topic
		self.mom.subscribe(ALIVE_TOPIC)
		self.aliveTmr = Timer(TMR_KEEPALIVE, self._tmrKeepAlive)
		self.aliveTmr.start()
		# Lauch discovery timer
		self.t = Timer(TMR_DISCOVERY, self._tmrDiscovery)
		self.t.start()
		
		while(1):
			try:
				req = self.mom.receive()
				if self._messageIsDiscovery(req) or self._messageIsKeepAlive(req):
					coordId = int(req.split(":")[-1])
					self._updateAliveTable(coordId)
					self._log("Discovered " + str(self.aliveCoords.keys()))
					
			except:
				break
		
		self._log("Discovery finished!")
	
	def _reqToProcessId(self, req):
		return req.split()[0].split(":")[0]
	
	def _reqToReqNumber(self, req):
		return req.split()[-1]
	
	def _getLastResponses(self, processId):
		return self.reqTable[processId][1]
	
	def _addReqResponses(self, req, responses):
		processId = self._reqToProcessId(req)
		reqNumber = self._reqToReqNumber(req)
		self.reqTable[processId] = (reqNumber, responses)
	
	def _reqIsNew(self, req):
		processId = self._reqToProcessId(req)
		if not processId in self.reqTable:
			return True
		reqNumber = self._reqToReqNumber(req)
		if reqNumber == self.reqTable[processId][0]:
			return False
		return True

	
	def _leaderState(self):
		os.write(1, "Pot " + str(self.potId) + ": Leveled up to leader!\n")
		coordTopic = COORDINATOR_TOPIC + str(self.potId)
		self.mom.subscribe(coordTopic)
		
		if self.potId != 1:
			# Tell proxies we have now another leader
			self.mom.publish(PROXIES_TOPIC, MSG_NEW_COORD_TOPIC + coordTopic)

		self.t = Timer(TMR_UPDATE_RESOURCES, self._tmrLeader)
		self.t.start()
		while(1):
			os.write(1, "Pot " + str(self.potId) + ": Waiting request...\n")
			req = self.mom.receive()
			if self._messageIsDiscovery(req):
				continue
				
			if self._reqIsNew(req):	
				os.write(1, "Pot " + str(self.potId) + ": Executing new request: " + req +"\n")
				responses = self.cnator.executeOrder(req)
				#self.cnator.printStatus()
				self.mom.publish(BACKUPS_TOPIC, req)
				self.cnator.sendResponses(responses)
				self.t.cancel()
				self.t = Timer(TMR_UPDATE_RESOURCES, self._tmrLeader)
				self.t.start()
				self._addReqResponses(req, responses)
				#os.write(1, "reqTable:" + str(self.reqTable) +"\n")
			else:
				os.write(1, "Pot " + str(self.potId) + ": RETRANSMITTING FOR: " + req +"\n")
				responses = self._getLastResponses(self._reqToProcessId(req))
				self.cnator.sendResponses(responses)
	
	def _tmrBackup(self):
		# If here, the leader is dead
		os.write(1, "Pot " + str(self.potId) + ": Leader is down!\n")
		self.leaderId = (self.leaderId % self.potAmount) + 1
		os.write(1, "Pot " + str(self.potId) + ": All hail new leader: " + str(self.leaderId) + "\n")
		#self.mom.publish(DISCOVERY_TOPIC, MSG_DISCOVERY + str(self.potId))
		os.kill(os.getpid(), signal.SIGINT)
	
	def _backupState(self):
		aliveLeaderId = self.leaderId
		self.t = Timer(TMR_LEADER_ALIVE, self._tmrBackup)
		self.t.start()
		self.mom.subscribe(BACKUPS_TOPIC)
		while aliveLeaderId == self.leaderId:
			try:
				os.write(1, "Pot " + str(self.potId) + ": Waiting...\n")
				req = self.mom.receive()
				if self._messageIsDiscovery(req):
					continue
					
				os.write(1, "Pot " + str(self.potId) + ": Received " + req + "\n")
				if req != MSG_LEADER_ALIVE:
					if self._reqIsNew(req):
						os.write(1, "Pot " + str(self.potId) + ": Executing " + req +"\n")					
						responses = self.cnator.executeOrder(req)
						#self.cnator.printStatus()
						self._addReqResponses(req, responses)
					
				self.t.cancel()
				self.t = Timer(TMR_LEADER_ALIVE, self._tmrBackup)
				self.t.start()
				#os.write(1, "reqTable:" + str(self.reqTable) +"\n")
			except:
				pass

		# If here, this potential is now the leader
		self.mom.unsubscribe(BACKUPS_TOPIC)
	
	def _tmrDefunct(self):
		# If here, one coordinator has died
		now = time.time()
		aliveOnes = {}
		for coordId in self.aliveCoords:
			if (now - self.aliveCoords[coordId]) > TMR_DEFUNCT:
				self._log("Coordinator " + str(coordId) + " has died!")
				self._log("Remaining are " + str(self.aliveCoords.keys()))
			else:
				aliveOnes[coordId] = self.aliveCoords[coordId]
		
		self.aliveCoords = aliveOnes
		self._launchDefunctTmr()
	
	def _launchDefunctTmr(self):
		# Retrieve the oldest time in aliveCoords table
		# (i.e. time related to the lest recent coordinator
		# in sending keepalive message) 
		if self.t is not None:
			self.t.cancel()
		if len(self.aliveCoords) > 0:
			oldest = min(self.aliveCoords[k] for k in self.aliveCoords)
			elapsed = time.time() - oldest
			self.t = Timer(max(TMR_DEFUNCT - elapsed, 0), self._tmrDefunct)
			self.t.start()
	
	
	def coordLoop(self):
		self._launchDefunctTmr()
		while(1):
			try:
				self._log("Waiting...")
				req = self.mom.receive()
				if self._messageIsDiscovery(req):
					coordId = int(req.split(":")[-1])
					self._updateAliveTable(coordId)
					self._log("Coordinator with id " + str(coordId) + " appeared")
					
				if self._messageIsKeepAlive(req):
					coordId = int(req.split(":")[-1])
					self._updateAliveTable(coordId)
					self._launchDefunctTmr()
					
			except:
				pass


p = Coordinator()
p.coordLoop()
