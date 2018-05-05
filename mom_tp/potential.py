#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys
from threading import *
import os
import signal

# Must be called as "python potential.py <potId> <potAmount>"


# ------------------------ Topics used ------------------------

DISCOVERY_TOPIC = "Museum/Coordinator/Learning"
RESOURCES_TOPIC = "Museum/Coordinator/Resources"
COORDINATOR_TOPIC = "Museum/Coordinator/Coordinator"

# -------------------------- Timers --------------------------

TMR_UPDATE_RESOURCES = 2.0
TMR_LEADER_ALIVE = 5.0




MSG_LEADER_ALIVE = "LEADER IS ALIVE"
MSG_DISCOVERY = "DIS"

class Potential:
	def __init__(self):
		self.potId = int(sys.argv[1])
		self.potAmount = int(sys.argv[2])
		self.mom = Mom()
		self._discoveryState()
		self.leaderId = 1
		self.t = None

		
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
			os.write(1, "Potential " + ownId + ": Received " + str(list(discovered)) + "\n")
			
		# If here, all potential coordinators were discovered
		os.write(1, "Potential " + ownId + ": Ready to rock!\n")
	
	def _tmrLeader(self):
		self.mom.publish(RESOURCES_TOPIC, MSG_LEADER_ALIVE)
		self.t = Timer(TMR_UPDATE_RESOURCES, self._tmrLeader)
		self.t.start()
		os.write(1, "Potential " + str(self.potId) + ": Sending leader alive message\n") 
	
	def _leaderState(self):
		os.write(1, "Potential " + str(self.potId) + ": Leveled up to leader!\n")
		self.mom.subscribe(COORDINATOR_TOPIC)
		self.t = Timer(TMR_UPDATE_RESOURCES, self._tmrLeader)
		self.t.start()
		while(1):
			os.write(1, "Potential " + str(self.potId) + ": Awaiting order...\n")
			req = self.mom.receive()
			if req[:len(MSG_DISCOVERY)] == MSG_DISCOVERY:
				continue
	
	def _tmrBackup(self):
		# If here, the leader is dead
		os.write(1, "Potential " + str(self.potId) + ": Leader is down!\n")
		self.leaderId = (self.leaderId % self.potAmount) + 1
		os.write(1, "Potential " + str(self.potId) + ": All hail new leader: " + str(self.leaderId) + "\n")
		#self.mom.publish(DISCOVERY_TOPIC, MSG_DISCOVERY + str(self.potId))
		os.kill(os.getpid(), signal.SIGINT)
	
	def _backupState(self):
		aliveLeaderId = self.leaderId
		self.t = Timer(TMR_LEADER_ALIVE, self._tmrBackup)
		self.t.start()
		self.mom.subscribe(RESOURCES_TOPIC)
		while aliveLeaderId == self.leaderId:
			try:
				os.write(1, "Potential " + str(self.potId) + ": Waiting...\n")
				s = self.mom.receive()
				if s[:len(MSG_DISCOVERY)] == MSG_DISCOVERY:
					continue
				os.write(1, "Potential " + str(self.potId) + ": Received " + s + "\n")
				if s == MSG_LEADER_ALIVE:
					self.t.cancel()
					self.t = Timer(TMR_LEADER_ALIVE, self._tmrBackup)
					self.t.start()
			except:
				pass

		# If here, this potential is now the leader
		self.mom.unsubscribe(RESOURCES_TOPIC)
	
	def potentialLoop(self):
		while(1):
			if self.leaderId == self.potId:
				self._leaderState()
			else:
				self._backupState()


p = Potential()
p.potentialLoop()
