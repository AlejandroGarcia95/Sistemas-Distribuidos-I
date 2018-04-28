#!/usr/bin/python
# -*- coding: utf-8 -*-

import sysv_ipc as sv
import os
import struct

# --------------------- Python MOM API --------------------- 



# ----------------------- Constants ------------------------

OC_CREATE = 1
OC_DESTROY = 2 
OC_PUBLISH = 3
OC_SUBSCRIBE = 4
OC_ACK_SUCCESS = 5
OC_ACK_FAILURE = 6
OC_DELIVERED = 7
OC_SEPPUKU = 8

PAYLOAD_SIZE = 255
TOPIC_LENGTH = 100

QUEUE_RESPONSER = "queue_resp.temp"
QUEUE_REQUESTER = "queue_req.temp"

MSQ_MAGIC_INT = 11	# For ftok

# --------------- Auxiliar MomMessage class ----------------

'''
Format of mom_message_t as declared in mom_general.h:

	long mtype;
	
	long local_id;
	long global_id;
	opcode_t opcode;
	char topic[TOPIC_LENGTH];
	
	char payload[PAYLOAD_SIZE];
	
'''

class MomMessage:
	def __init__(self, localId=0, globalId=0, opcode=0):
		self.localId = localId
		self.globalId = globalId
		self.opcode  = opcode
		self.topic = "".ljust(TOPIC_LENGTH, "\0")
		self.payload = "".ljust(PAYLOAD_SIZE, "\0")
	
	def setTopic(self, topic):
		self.topic = topic.ljust(TOPIC_LENGTH, "\0")
		
	def setPayload(self, payload):
		self.payload = payload.ljust(PAYLOAD_SIZE, "\0")
	
	def getStringRepresentation(self):
		# Pack the message!
		r = struct.pack(b'@lli', self.localId, self.globalId, self.opcode)
		r = r + self.topic + self.payload
		return r
	
	def setFromStringRepresentation(self, s):
		TEXT_START = 12
		(self.localId, self.globalId, self.opcode) = struct.unpack(b'@lli', s[0:TEXT_START])
		self.topic = s[TEXT_START:TEXT_START+TOPIC_LENGTH]
		self.payload = s[TEXT_START+TOPIC_LENGTH:TEXT_START+TOPIC_LENGTH+PAYLOAD_SIZE]
	
	def getGlobalId(self):
		return self.globalId
		
	def getOpcode(self):
		return self.opcode
		
	def getPayload(self):
		return self.payload
		
	def printMessage(self):
		oc_str = ["", "CREATE", "DESTROY", "PUBLISH", "SUBSCRIBE", "ACK_SUCCESS", "ACK_FAILURE", "DELIVERED", "SEPPUKU"]
		print("Local id: " + str(self.localId))
		print("Global id: " + str(self.globalId))
		print("Opcode: " + oc_str[self.opcode])
		print("Topic: " + self.topic + ".")
		print("Payload: " + self.payload + ".")

# ----------- The fantabolously magic Mom class -----------

class Mom:
	def __init__(self):
		self.localId = os.getpid()
		self.globalId = os.getpid()
		
		# Retrieve msqs
		keyReq = sv.ftok(QUEUE_REQUESTER, MSQ_MAGIC_INT, True)
		keyResp = sv.ftok(QUEUE_RESPONSER, MSQ_MAGIC_INT, True)
		self.msqReq = sv.MessageQueue(keyReq, flags=sv.IPC_CREAT, mode=0644)
		self.msqResp = sv.MessageQueue(keyResp, flags=sv.IPC_CREAT, mode=0644)
		
		# Retrieve globalId
		m = MomMessage(self.localId, self.globalId, OC_CREATE)
		response = self._sendMessageToDaemon(m)
		ropc = response.getOpcode()
		if (ropc != OC_ACK_FAILURE) and (ropc != OC_ACK_SUCCESS):
			print(str(os.getpid()) + ": MOM CRITICAL ON CREATING: Daemon answer was not ACK!\n")
			self = None
			return
		if ropc == OC_ACK_FAILURE:
			print(str(os.getpid()) + ": Error creating mom, could not register on broker\n")
			self = None
			return
		self.globalId = response.getGlobalId()
		
	def _topicIsValid(self, topic):
		if(len(topic) == 0):
			printf(str(os.getpid()) + ": Error, topic received has zero length!\n", )
			return False;
		
		if(len(topic) > TOPIC_LENGTH):
			printf(str(os.getpid()) + ": Error, topic cannot exceed lenght: " + str(TOPIC_LENGTH) + "!\n")
			return False;
		
		for c in topic:
			if((c == ' ') or (c == '.') or (c == ':') or (c == '!')):
				printf(str(os.getpid()) + ": Error, topic cannot have any space, . , : or !\n")
				return False;

		return True;
	
	def _sendMessageToDaemon(self, m):
		# Send message
		self.msqReq.send(m.getStringRepresentation(), type=self.localId)
		# Await response
		r, _ = self.msqResp.receive(type=self.localId)
		response = MomMessage()
		response.setFromStringRepresentation(r)
		return response
	
	def publish(self, topic, msg):
		if (topic is None) or (msg is None):
			print(str(os.getpid()) + ": Invalid argument in mom_publish: None received\n", );
			return False
		if not self._topicIsValid(topic):
			return False
		# Write and read publish message
		m = MomMessage(self.localId, self.globalId, OC_PUBLISH)
		m.setTopic(topic)
		m.setPayload(msg)
		response = self._sendMessageToDaemon(m)
		ropc = response.getOpcode()
		if (ropc != OC_ACK_FAILURE) and (ropc != OC_ACK_SUCCESS):
			print(str(os.getpid()) + ": MOM CRITICAL ON PUBLISHING: Daemon answer was not ACK!\n")
			self = None
			return

		return (ropc == OC_ACK_SUCCESS)
		
	def receive(self):
		# Locally receive message from daemon requester
		r, _ = self.msqResp.receive(type=self.globalId)
		response = MomMessage()
		response.setFromStringRepresentation(r)
		ropc = response.getOpcode()
		if (ropc != OC_DELIVERED):
			print(str(os.getpid()) + ": MOM CRITICAL ON RECEIVING: Daemon has not delivered message!\n")
			return None
		return response.getPayload()
		
	def subscribe(self, topic):
		if (topic is None):
			print(str(os.getpid()) + ": Invalid argument in mom_subscribe: None received\n", );
			return False
		if not self._topicIsValid(topic):
			return False
		# Write and read subscribe message
		m = MomMessage(self.localId, self.globalId, OC_SUBSCRIBE)
		m.setTopic(topic)
		response = self._sendMessageToDaemon(m)
		ropc = response.getOpcode()
		if (ropc != OC_ACK_FAILURE) and (ropc != OC_ACK_SUCCESS):
			print(str(os.getpid()) + ": MOM CRITICAL ON SUBSCRIBING: Daemon answer was not ACK!\n")
			self = None
			return

		return (ropc == OC_ACK_SUCCESS)
	
	def __del__(self):
		# Send destroy message
		m = MomMessage(self.localId, self.globalId, OC_DESTROY)
		response = self._sendMessageToDaemon(m)
