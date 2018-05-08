#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
from threading import *
import os
import signal
from coordinator import *

# Must be called as "python potential.py <potId> <potAmount>"


# ------------------------ Topics used ------------------------

DISCOVERY_TOPIC = "Museum/Coordinator/Learning"
BACKUPS_TOPIC = "Museum/Coordinator/Backups"
COORDINATOR_TOPIC = "Museum/Coordinator/Coordinator"
PROXIES_TOPIC = "Museum/Coordinator/Proxies"

# -------------------------- Timers --------------------------

TMR_UPDATE_RESOURCES = 2.0
TMR_LEADER_ALIVE = 5.0




MSG_LEADER_ALIVE = "LEADER IS ALIVE"
MSG_DISCOVERY = "DIS"
MSG_NEW_COORD_TOPIC = "NEWTOPIC "

class Potential:
	def __init__(self):
		self.potId = int(sys.argv[1])
		self.potAmount = int(sys.argv[2])
		self.mom = Mom()
		self._discoveryState()
		self.leaderId = 1
		self.t = None
		self.cnator = Coordinator(self.mom)
		self.reqTable = {}

	def _messageIsDiscovery(self, m):
		return (m[:len(MSG_DISCOVERY)] == MSG_DISCOVERY)
		
	def _discoveryState(self):
		# Subscribe to discovery topic
		self.mom.subscribe(DISCOVERY_TOPIC)
		# Find out all other potential coordinators
		ownId = str(self.potId)
		discovered = { ownId }
		discoveredStr = MSG_DISCOVERY + "".join(p + " " for p in discovered)
		self.mom.publish(DISCOVERY_TOPIC, discoveredStr)
		while len(discovered) < self.potAmount:
			newDiscovered = set(self.mom.receive()[len(MSG_DISCOVERY):].split())
			if len(discovered - newDiscovered) > 0:
				newDiscovered.update(discovered)
				discoveredStr = MSG_DISCOVERY + "".join(p + " " for p in newDiscovered)
				self.mom.publish(DISCOVERY_TOPIC, discoveredStr)
			discovered = newDiscovered
			os.write(1, "Pot " + ownId + ": Received " + str(list(discovered)) + "\n")
			
		# If here, all potential coordinators were discovered
		os.write(1, "Pot " + ownId + ": Ready to rock!\n")
	
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
	
	def _tmrLeader(self):
		self.mom.publish(BACKUPS_TOPIC, MSG_LEADER_ALIVE)
		self.t = Timer(TMR_UPDATE_RESOURCES, self._tmrLeader)
		self.t.start()
		os.write(1, "Pot " + str(self.potId) + ": Sending leader alive message\n") 
	
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
	
	def potentialLoop(self):
		while(1):
			if self.leaderId == self.potId:
				self._leaderState()
			else:
				self._backupState()


p = Potential()
p.potentialLoop()
