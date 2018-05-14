#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys
from threading import *
import os
import signal
import time

# Must be called as "python new_coordinator.py <cId> <cAmount>"

REDUNDANCY = 1

# ------------------------ Topics used ------------------------

ALIVE_TOPIC = "Philo/Coordinator/Alive"
COORDINATOR_TOPIC = "Philo/Coordinator/Coordinator"
PROXIES_TOPIC = "Philo/Coordinator/Proxies"
LEARN_TOPIC = "Philo/Coordinator/Learning"

# -------------------------- Timers --------------------------

TMR_DISCOVERY = 14.0
TMR_KEEPALIVE = 5.0
TMR_DEFUNCT = 12.5

# ------------------------- Messages -------------------------

MSG_KEEPALIVE = "ALIVE"
MSG_DISCOVERY = "DIS"
MSG_NEW_COORD_TOPIC = "NEWTOPIC "
MSG_RESOURCE = "RES"
MSG_REQTABLE = "REQ"
MSG_RES_SEP = "#"

MSG_RES_SIZE = 200

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
					os.write(1, "SOMETHING WENT TERRIBLY WRONG: SEM HASNT PROPERLY WAIT\n") 
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
			return []
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
	
	def imposeSem(self, semName, semValue, semProcessList):
		self.semTable[semName] = (semValue, semProcessList)
	
	def imposeShm(self, shmName, shmValue):
		self.shmTable[shmName] = shmValue
	
	
	def getSemTable(self):
		return self.semTable
	
	def getShmTable(self):
		return self.shmTable
		
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
		self.aliveTmr = None
		self.t = None
		self.db = ResourcesDB(self.mom)
		self.reqTable = {}
		self._log("Coord up! Entering discovery mode...")
		self._discoveryState()
		self.mom.unsubscribe(LEARN_TOPIC + str(self.cId))

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
	
	def _messageIsResources(self, m):
		return (m[:len(MSG_RESOURCE)] == MSG_RESOURCE)
			
	def _messageIsReqTable(self, m):
		return (m[:len(MSG_REQTABLE)] == MSG_REQTABLE)
		
	def _tmrDiscovery(self):
		# If here, I've ended my discovery time
		os.kill(os.getpid(), signal.SIGINT)
	
	def _tmrKeepAlive(self):
		self._log("Sending keepalive message")
		self.mom.publish(ALIVE_TOPIC, MSG_KEEPALIVE + ":" + str(self.cId))
		self.aliveTmr = Timer(TMR_KEEPALIVE, self._tmrKeepAlive)
		self.aliveTmr.start()
	
	def _parseResources(self, msg):
		resList = msg.split(MSG_RES_SEP)
		for _res in resList:
			thisRes = _res.split(" ")
			if len(thisRes) < 3:
				continue
			resName = thisRes[1]
			resValue = int(thisRes[2])
			if thisRes[0].split(":")[1] == "SEM":
				processList = []
				for i in range(3, len(thisRes)):
					processList.append(thisRes[i])
				self.db.imposeSem(resName, resValue, processList)
			elif thisRes[0].split(":")[1] == "SHM":
				self.db.imposeShm(resName, resValue)
		self.db.printStatus()
	
	def _parseReqTable(self, msg):
		self._log("Here1 with " + msg)
		lastRespList = msg.split(MSG_RES_SEP)
		for _resp in lastRespList:
			self._log("Here2 with " + _resp)
			thisRes = _resp.split(" ")
			if len(thisRes) < 3:
				continue
			reqNumber = thisRes[1]
			processId = thisRes[0].split(":")[1]
			self._log("Here3 with " + processId + " " + reqNumber)
			# Loop response list
			responses = []
			for i in range(2, len(thisRes), 2):
				aux = thisRes[i + 1]
				if aux == "None":
					aux = None
				elif aux == "True": 
					aux = True
				elif aux == "False": 
					aux = False
				responses.append( (thisRes[i], aux) )
			
			if not processId in self.reqTable:
				self.reqTable[processId] = (reqNumber, responses)
			elif int(self.reqTable[processId][0]) < int(reqNumber):
				self.reqTable[processId] = (reqNumber, responses)
				
		self._log("ReqTable: " + str(self.reqTable))
	
	def _discoveryState(self):
		# Subscribe to keepalive and learning topic
		self.mom.subscribe(ALIVE_TOPIC)
		self.mom.subscribe(LEARN_TOPIC + str(self.cId))
		# Anounce I'm here and learning
		self.mom.publish(ALIVE_TOPIC, MSG_DISCOVERY + ":" + str(self.cId))
		self.aliveTmr = Timer(TMR_KEEPALIVE, self._tmrKeepAlive)
		self.aliveTmr.start()
		# Lauch discovery timer
		startime = time.time()
		self.t = Timer(TMR_DISCOVERY, self._tmrDiscovery)
		self.t.start()
		
		while(1):
			try:
				req = self.mom.receive()
				if self._messageIsDiscovery(req) or self._messageIsKeepAlive(req):
					coordId = int(req.split(":")[-1])
					self._updateAliveTable(coordId)
					self._log("Discovered " + str(self.aliveCoords.keys()))
				
				elif self._messageIsResources(req):
					self._parseResources(req)
				elif self._messageIsReqTable(req):
					self._parseReqTable(req)
					
				now = time.time()
				self._log("Elapsed: " + str(now - startime))
					
			except:
				now = time.time()
				self._log("Hey, elapsed time was " + str(now - startime))
				break
		
		try:
			self._log("Discovery finished!")
		except:
			pass
	
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
	
	def _tmrBackup(self):
		# If here, the leader is dead
		os.write(1, "Pot " + str(self.potId) + ": Leader is down!\n")
		self.leaderId = (self.leaderId % self.potAmount) + 1
		os.write(1, "Pot " + str(self.potId) + ": All hail new leader: " + str(self.leaderId) + "\n")
		#self.mom.publish(DISCOVERY_TOPIC, MSG_DISCOVERY + str(self.potId))
		os.kill(os.getpid(), signal.SIGINT)
	
	def _tmrDefunct(self):
		# If here, one coordinator has died
		now = time.time()
		aliveOnes = {}
		for coordId in self.aliveCoords:
			if (now - self.aliveCoords[coordId]) > TMR_DEFUNCT:
				self._log("Coordinator " + str(coordId) + " has died!")
			else:
				aliveOnes[coordId] = self.aliveCoords[coordId]
		
		self.aliveCoords = aliveOnes
		self._log("Remaining are " + str(self.aliveCoords.keys()))
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
	
	def _coordIdToTopicId(self, coordId):
		topics = []
		for r in range(0, REDUNDANCY + 1):
			topicId = (coordId + r) % self.cAmount
			if topicId == 0:
				topicId = self.cAmount
			topics.append(topicId)
		return topics	
		
	def _topicIdToCoordId(self, topicId):
		coords = []
		for r in range(0, REDUNDANCY + 1):
			coordId = (topicId - r) % self.cAmount
			if coordId == 0:
				coordId = self.cAmount
			coords.append(coordId)
		return coords
	
	def _hashResName(self, resName):
		return ((hash(resName) % self.cAmount) + 1)
	
	# "processId:<SHM/SEM> <ACTION> <NAME> <VALUE> <REQ NUMBER>"
	# Works hashing the <NAME> field, returns -1 on error
	def _hashRequest(self, req):
		lReq = req.split()
		if len(lReq) < 3:
			return -1
		resName = lReq[2].rstrip('\x00').upper()
		return self._hashResName(resName)
	
	def _resourceIsMine(self, req):
		topicId = self._hashRequest(req)
		if topicId < 0:
			return False
		myTopics = self._coordIdToTopicId(self.cId)
		return (topicId in myTopics)
	
	def _semToStr(self, semName, semTuple):
		semStr = MSG_RESOURCE + ":SEM " + semName + " "
		semStr = semStr + str(semTuple[0]) 
		for processId in semTuple[1]:
			semStr = semStr + " " + str(processId)
		return semStr
	
	def _shmToStr(self, shmName, shmValue):
		shmStr = MSG_RESOURCE + ":SHM " + shmName + " "
		shmStr = shmStr + str(shmValue)
		return shmStr
	
	def _lastReqToStr(self, processId, reqNumber, responses):
		lastReqStr = MSG_REQTABLE + ":" + str(processId) + " "
		lastReqStr = lastReqStr + str(reqNumber) 
		for r in responses:
			lastReqStr = lastReqStr + " " + str(r[0]) + " " + str(r[1])
		return lastReqStr
	
	def _sendResToNewCoord(self, coordId):
		topics = self._coordIdToTopicId(coordId)
		self._log("Coord " + str(coordId) + " topics: " + str(topics))
		# First send sems
		semTable = self.db.getSemTable()
		payload = ""
		for sk in semTable:
			if self._hashResName(sk) in topics:	
				semStr = self._semToStr(sk, semTable[sk])
				if (len(payload) + len(semStr)) > MSG_RES_SIZE:
					self.mom.publish(LEARN_TOPIC + str(coordId), payload)
					self._log("Sending " + payload)
					payload = semStr + MSG_RES_SEP
				else:
					payload = payload + semStr + MSG_RES_SEP 
					
		if len(payload) > 1:
			self.mom.publish(LEARN_TOPIC + str(coordId), payload)
			self._log("Sending " + payload)
		
		# Then send shms	
		payload = ""
		shmTable = self.db.getShmTable()
		for sk in shmTable:
			if self._hashResName(sk) in topics:
				shmStr = self._shmToStr(sk, shmTable[sk])
				if (len(payload) + len(shmStr)) > MSG_RES_SIZE:
					self.mom.publish(LEARN_TOPIC + str(coordId), payload)
					self._log("Sending " + payload)
					payload = shmStr + MSG_RES_SEP
				else:
					payload = payload + shmStr + MSG_RES_SEP
					
		if len(payload) > 1:
			self.mom.publish(LEARN_TOPIC + str(coordId), payload)
			self._log("Sending " + payload)
		# Finally send reqTable
		payload = ""
		for pId in self.reqTable:
			lastReqStr = self._lastReqToStr(pId, self.reqTable[pId][0], self.reqTable[pId][1])
			if (len(payload) + len(lastReqStr)) > MSG_RES_SIZE:
				self.mom.publish(LEARN_TOPIC + str(coordId), payload)
				self._log("Sending " + payload)
				payload = lastReqStr + MSG_RES_SEP
			else:
				payload = payload + lastReqStr + MSG_RES_SEP
		if len(payload) > 1:
			self.mom.publish(LEARN_TOPIC + str(coordId), payload)
			self._log("Sending " + payload)			
	
	def _haveToSendResponse(self, req):
		topicId = self._hashRequest(req)
		if topicId < 0:
			return False
		# Retrieve all coordinators related to that resource
		coords = self._topicIdToCoordId(topicId)
		if not self.cId in coords:
			return False
		# If here, I have to answer if I'm the biggest alive
		livingCoords = self.aliveCoords.keys()
		livingCoords.append(self.cId)
		coordsForRes = [c for c in coords if c in livingCoords]
		return (self.cId == max(coordsForRes))
	
	def coordLoop(self):
		self._launchDefunctTmr()
		# Subscribe to coordinator topic
		self.mom.subscribe(COORDINATOR_TOPIC)
		while(1):
			try:
				self._log("Waiting...")
				req = self.mom.receive()
				if self._messageIsDiscovery(req):
					coordId = int(req.split(":")[-1])
					self._sendResToNewCoord(coordId)
					self._updateAliveTable(coordId)
					self._log("Coordinator with id " + str(coordId) + " appeared")
				
				elif self._messageIsKeepAlive(req):
					coordId = int(req.split(":")[-1])
					self._updateAliveTable(coordId)
					self._launchDefunctTmr()
				
				elif self._messageIsResources(req):
					self._log("Resources shouldnt be here!")
					
				elif self._messageIsReqTable(req):
					self._log("ReqTable shouldnt be here!")
				
				else: # Request		
					if self._resourceIsMine(req):
						if self._reqIsNew(req):
							self._log("Executing new request: " + req)
							responses = self.db.executeOrder(req)
							self.db.printStatus()
							if self._haveToSendResponse(req):
								self.db.sendResponses(responses)
							self._addReqResponses(req, responses)
						elif self._haveToSendResponse(req):
							self._log("RETRANSMITTING FOR " + req)
							responses = self._getLastResponses(self._reqToProcessId(req))
							self._log("RESPONSES ARE: " + str(responses))
							self.db.sendResponses(responses)
					
			except:
				break


p = Coordinator()
p.coordLoop()
