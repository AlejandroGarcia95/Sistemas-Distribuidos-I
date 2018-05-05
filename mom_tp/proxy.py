#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys

COORDINATOR_TOPIC = "Museum/Coordinator/Coordinator"

# ----------- Python Potential Coordinator -----------

class Proxy:
	def __init__(self):
		self.mom = Mom()
		self.potId = ""
		self._setPotentialId(sys.argv[1])
		self.topic = "Museum/" + self.potId
		self.mom.subscribe(self.topic)
		
	def _setPotentialId(self, priority):
		self.potId =  str(os.getpid() + int(priority) * 100000)
	
	def proxyLoop(self):
		reqNumber = 1
		while(1):
			try:
				msg = raw_input()
			except EOFError:
				break
			msg = self.potId + ":" + msg + " " + str(reqNumber)
			self.mom.publish(COORDINATOR_TOPIC, msg)
			msg = self.mom.receive()
			reqNumber += 1
			try:
				os.write(1, msg)
			except OSError:
				break



# Main

prx = Proxy()
prx.proxyLoop()
