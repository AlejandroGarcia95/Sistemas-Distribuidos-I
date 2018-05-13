#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys
import os
from threading import *
import signal
import fcntl
import time

COORDINATOR_TOPIC = "Philo/Coordinator/Coordinator"

TMR_PROXY = 21.0

# ----------- Python Potential Coordinator -----------

class Proxy:
	def __init__(self):
		self.mom = Mom()
		self.proxyId = ""
		self._setProxyId(sys.argv[1])
		self.topic = "Philo/" + self.proxyId
		self.mom.subscribe(self.topic)
		self.t = None
		self.lastRequest = None
		
	def _setProxyId(self, priority):
		self.proxyId =  str(os.getpid() + int(priority) * 100000)
	
	def _tmrProxy(self):
		self.mom.publish(COORDINATOR_TOPIC, self.lastRequest)
		self.t = Timer(TMR_PROXY, self._tmrProxy)
		self.t.start()
		
	
	def proxyLoop(self):
		reqNumber = 1
		while(1):
			try:
				msg = raw_input()
			except EOFError:
				break
			self.lastRequest = self.proxyId + ":" + msg + " " + str(reqNumber)
			# Retransmission timer
			self.t = Timer(TMR_PROXY, self._tmrProxy)
			self.t.start()
			self.mom.publish(COORDINATOR_TOPIC, self.lastRequest)
	
			msg = self.mom.receive()
			self.t.cancel()
			reqNumber += 1
			
			try:
				os.write(1, msg)
			except OSError:
				break



# Main

prx = Proxy()
prx.proxyLoop()
