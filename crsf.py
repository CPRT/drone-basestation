import time
import serial

# CRSF Broadcast Frame (sync bytes <= 0x27 )
# Offset |        Usage        | CRC | Comment
#   0    |      sync byte      | No  | uint8, e.g. 0xEE -> to TX module, 0xC8 -> to FC, 0xEC -> FC to RX
#   1    |    frame length     | No  | uint8, entire frame size -2 (allowed values: 2-62)
#   2    |         type        | Yes | uint8, e.g. 0x02 -> GPS data
#   3    |       payload       | Yes | payload with length n bytes
#  n+4   |      checksum       | No  | uint8, basic CRC8 using polynomial 0xD5 (-> DVB-S2)

# CRSF Extended Frame (sync bytes >= 0x28 )
# Offset |        Usage        | CRC | Comment
#   0    |      sync byte      | No  | uint8, e.g. 0xEE -> to TX module, 0xC8 -> to FC, 0xEC -> FC to RX
#   1    |    frame length     | No  | uint8, entire frame size -2 (allowed values: 2-62)
#   2    |         type        | Yes | uint8, e.g. 0x02 -> GPS data
#   3    | destination address | Yes | uint8, e.g. 0xC8 -> Flight Controller
#   4    |    origin address   | Yes | uint8, e.g. 0xEA -> Remote Control
#   5    |       payload       | Yes | payload with length n bytes
#  n+6   |      checksum       | No  | uint8, basic CRC8 using polynomial 0xD5 (-> DVB-S2)

############################################
############# Helper functions #############
############################################

def calculate_DVB_S2_checksum(data) -> int:
	checksum = 0x00
	for byte in data:
		checksum ^= byte
		for _ in range(8):
			if checksum & 0x80: checksum = (checksum << 1) ^ 0xD5
			else: checksum <<= 1
			checksum &= 0xFF 
	return checksum

def pack(rcChannelValues:list[int]) -> list[int]:
	packedValues = [0]*22
	tempValues = [0]*16*11
	if len(rcChannelValues) == 16:
		if min(rcChannelValues) >= 1000 and max(rcChannelValues) <= 2000:
			# Here we encode the 16 channels of 11 bits into 22 bytes of 8 bits
			# More about this here: https://github.com/tbs-fpv/tbs-crsf-spec/blob/main/crsf.md#0x16-rc-channels-packed-payload
			# As a first step, we map all channel values according to the CRSF specification and string them into one large binary array of 16*11 = 176 bits
			# PAY ATTENTION TO WHAT MAPPING YOUR TRANSMITTER USES! More about this topic here: https://www.expresslrs.org/software/switch-config/
			for index, channelValue in enumerate(reversed(rcChannelValues)):
				tempValues[index*11:(index+1)*11] = f'{int(round((channelValue - 1500) * 8 / 5 + 992,0)):011b}'
			tempValues = "".join(tempValues)
			# Then we segment this array into 22 bytes of 8 bits each and transform the binary values into INTs
			for index in range(len(packedValues)):
				packedValues[index] = int(tempValues[index*8:(index+1)*8], 2)
			packedValues = list(reversed(packedValues))
		else:
			print('There is a channel with invalid value (valid range: 1000-2000)')
	else:
		print('There are not 16 channels in this list')
	return packedValues

def createMessage(syncByte:int, messageType:int, payload:list[int]) -> list[int]:
	message = (len(payload)+4)*[0]
	message[0] = int(syncByte)
	message[1] = int(len(payload)+2)
	message[2] = int(messageType)
	message[3:-1] = payload
	message[-1] = calculate_DVB_S2_checksum(message[2:-1])
	return message

##############################################
######### Initialize some parameters #########
##############################################

SERIAL_PORT = '/dev/ttyAMA2'
BAUD_RATE = 921600
serialTransmitter = serial.Serial(SERIAL_PORT, BAUD_RATE)

_lasttimeWrite = time.monotonic()
dtWrite = 0.004	# in seconds -> ~250Hz
syncByte = 0xEE
frameLength = 22+2
messageType = 0x16

#######################################
######### Program starts here #########
#######################################
i = 0
while True:
	if time.monotonic() - _lasttimeWrite >= dtWrite:
		_lasttimeWrite = time.monotonic()
		rcChannelValues = [1500, 1500, 1000, 1500, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
		payload = pack(rcChannelValues)
		message = createMessage(syncByte, messageType, payload)
		serialTransmitter.write(message)
		i += 1
	if i >= 250:
		break

message = createMessage(syncByte, 0x2D, [0xEE, 0xEF, 1, 4]) # configure packet rate (4 = 500 Hz)
print("".join("{:02x}".format(a) for a in message))
serialTransmitter.write(message)
i = 0
while True:
	if time.monotonic() - _lasttimeWrite >= dtWrite:
		_lasttimeWrite = time.monotonic()
		rcChannelValues = [1500, 1500, 1000, 1500, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000]
		payload = pack(rcChannelValues)
		message = createMessage(syncByte, messageType, payload)
		serialTransmitter.write(message)
		i += 1
	if i >= 1000:
		break
