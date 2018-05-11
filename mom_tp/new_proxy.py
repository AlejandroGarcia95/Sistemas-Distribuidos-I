#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys
import os
from threading import *
import signal
import fcntl
import time

COORDINATOR_TOPIC = "Philo/Coordinator/Coordinator1"
PROXIES_TOPIC = "Philo/Coordinator/Proxies"

COORD_TOPIC_FILE = "CoordTopic.txt"

TMR_PROXY = 21.0

MSG_NEW_COORD_TOPIC = "NEWTOPIC "

# ----------- Python Potential Coordinator -----------

class Proxy:
	def __init__(self):
		self.mom = Mom()
		self.potId = ""
		self._setPotentialId(sys.argv[1])
		self.topic = "Philo/" + self.potId
		self.mom.subscribe(PROXIES_TOPIC)
		self.mom.subscribe(self.topic)
		self.t = None
		self.lastRequest = None
		self.coordTopicFile = None
		if os.path.exists(COORD_TOPIC_FILE):
			self._lockCoordTopicFile()
			self.coordTopic = self.coordTopicFile.readline()
			self._unlockCoordTopicFile()
		else:
			os.system("touch " + COORD_TOPIC_FILE)
			self._lockCoordTopicFile()
			self.coordTopicFile.write(COORDINATOR_TOPIC)
			self.coordTopic = COORDINATOR_TOPIC
			self._unlockCoordTopicFile()
			
		
	def _lockCoordTopicFile(self):
		self.coordTopicFile = open(COORD_TOPIC_FILE, "r+")
		while True:
			try:
				fcntl.flock(self.coordTopicFile, fcntl.LOCK_EX | fcntl.LOCK_NB)
				break
			except IOError as e:
				# raise on unrelated IOErrors
				if e.errno != errno.EAGAIN:
					raise
				else:
					time.sleep(0.01)
	
	def _unlockCoordTopicFile(self):
		fcntl.flock(self.coordTopicFile, fcntl.LOCK_UN)
		self.coordTopicFile.close()
		
	def _setPotentialId(self, priority):
		self.potId =  str(os.getpid() + int(priority) * 100000)
	
	def _tmrProxy(self):
		self.mom.publish(self.coordTopic, self.lastRequest)
		self.t = Timer(TMR_PROXY, self._tmrProxy)
		self.t.start()
			
	def _messageIsNewTopic(self, m):
		return (m[:len(MSG_NEW_COORD_TOPIC)] == MSG_NEW_COORD_TOPIC)
	
	def _setNewCoordTopic(self, m):
		self.coordTopic = m.split()[-1]
		self._lockCoordTopicFile()
		self.coordTopicFile.write(self.coordTopic)
		self._unlockCoordTopicFile()
	
	def proxyLoop(self):
		reqNumber = 1
		while(1):
			try:
				msg = raw_input()
			except EOFError:
				break
			self.lastRequest = self.potId + ":" + msg + " " + str(reqNumber)
			# Retransmission timer
			self.t = Timer(TMR_PROXY, self._tmrProxy)
			self.t.start()
			self.mom.publish(self.coordTopic, self.lastRequest)
			while(1):
				msg = self.mom.receive()
				if self._messageIsNewTopic(msg):
					self._setNewCoordTopic(msg)
				else:
					break
			self.t.cancel()
			reqNumber += 1
			try:
				os.write(1, msg)
			except OSError:
				break



# Main

prx = Proxy()
prx.proxyLoop()
