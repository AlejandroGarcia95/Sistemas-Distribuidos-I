#!/usr/bin/python
# -*- coding: utf-8 -*-

from mom import *
import sys

# ----------- Python Potential Coordinator -----------

class Potential:
	def __init__(self):
		self.mom = Mom()
		self.topic = "Museum/" + self._getPotentialId()
		self.mom.subscribe(self.topic)
		
	def _getPotentialId(self):
		return str(os.getpid())
	
	def potentialLoop(self):
		while(1):
			try:
				msg = raw_input()
			except EOFError:
				break
			msg = self._getPotentialId() + ":" + msg
			self.mom.publish("Museum/Coordinator", msg)
			msg = self.mom.receive()
			try:
				os.write(1, msg)
			except OSError:
				break



# Main

pot = Potential()
pot.potentialLoop()
