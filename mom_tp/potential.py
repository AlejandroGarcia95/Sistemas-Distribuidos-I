#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys

# ----------- Python Potential Coordinator -----------

class Potential:
	def __init__(self):
		self.mom = Mom()
		self.potId = ""
		self._setPotentialId(sys.argv[1])
		self.topic = "Museum/" + self.potId
		self.mom.subscribe(self.topic)
		
	def _setPotentialId(self, priority):
		self.potId =  str(os.getpid() + int(priority) * 100000)
	
	def potentialLoop(self):
		while(1):
			try:
				msg = raw_input()
			except EOFError:
				break
			msg = self.potId + ":" + msg
			self.mom.publish("Museum/Coordinator", msg)
			msg = self.mom.receive()
			try:
				os.write(1, msg)
			except OSError:
				break



# Main

pot = Potential()
pot.potentialLoop()
