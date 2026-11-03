import bleak
import asyncio
import cbor2
import hashlib
import struct
smpCharectaristicUuid = "DA2E7828-FBCE-4E01-AE9E-261174997C48"
newNrfImageName = "zephyr.signed.bin"
VERSION_OF_PROTOCOL = 1 # New Simple Management Protocol (SMP) version (old one is 0)
OP_BYTE_OFFSET = 3 

imageUploadRequest= {"OP":2, "GroupID":1, "CommandID":1}
stateOfImage = {"OP": 0, "GroupID": 1, "CommandID":0}
setStateOfImage = {"OP": 2, "GroupID": 1, "CommandID":0}
systemResetRequest = {"OP": 2, "GroupID": 0, "CommandID": 5}

responses = asyncio.Queue()

async def cb(_, data: bytearray):
    responses.put_nowait(bytes(data))


def createPacket(request : dict, message : dict, seq: int):
    serializedMessage = cbor2.dumps(message)
    header = struct.pack(">BBHHBB",
                        (VERSION_OF_PROTOCOL << OP_BYTE_OFFSET) | request["OP"],
                        0, # flags are always to be set to 0, per documentation https://docs.zephyrproject.org/latest/services/device_mgmt/smp_protocol.html#mcumgr-smp-protocol-group-ids
                        len(serializedMessage),
                        request["GroupID"],
                        seq,
                        request["CommandID"])
    
    return header + serializedMessage

def fullPacket(image: bytes, offset: int, seq: int, maxMTU: int, sha: bytes) -> bytes:
    maxSizeToBeSent = min(len(image) - offset, maxMTU)
    payload = {}
    if offset == 0:
        payload.update({"image": 0, "len": len(image), "sha": sha})
    dataToBeSent = image[offset:offset + maxSizeToBeSent]
    payload.update({"off": offset, "data": dataToBeSent})
    packet = createPacket(imageUploadRequest, payload, seq)

    if len(packet) > maxMTU:
        payload.update({"data" : image[offset:offset + maxSizeToBeSent - len(packet) + maxMTU]})
        packet = createPacket(imageUploadRequest, payload, seq)

    return packet
    

# byte 0 = Reserved + Version + Operation
# byte 1 = flags
# byte 2 & 3 = length
# byte 4 & 5 = group ID
# byte 6 = sequence number
# byte 7 = command ID

async def confirmUpload(client: bleak.BleakClient):
    stateOfImageRequestPacket = createPacket(stateOfImage, {}, 0)
    await client.write_gatt_char(smpCharectaristicUuid, stateOfImageRequestPacket, response=False)
    packet = await asyncio.wait_for(responses.get(), timeout=5)
    dataLength = 8 + struct.unpack(">H", packet[2:4])[0]
    while len(packet) < dataLength:
        packet += await asyncio.wait_for(responses.get(), timeout=5)
    state = cbor2.loads(packet[8:])

    slot1 = state["images"][1]
    setStateOfImageRequestBody = {"hash": slot1["hash"], "confirm": True}
    setStateOfImageRequestPacket = createPacket(setStateOfImage, setStateOfImageRequestBody, 0)
    await client.write_gatt_char(smpCharectaristicUuid, setStateOfImageRequestPacket, response=False)
    
    packet = await asyncio.wait_for(responses.get(), timeout=5)
    dataLength = 8 + struct.unpack(">H", packet[2:4])[0]
    while len(packet) < dataLength:
        packet += await asyncio.wait_for(responses.get(), timeout=5)
    rsp = cbor2.loads(packet[8:])

    if("err" in rsp):
        print("Error in setting the new image to be booted.")
        return

    resetPacket = createPacket(systemResetRequest, {}, 0)
    await client.write_gatt_char(smpCharectaristicUuid, resetPacket, response=False)

async def main():
    async with bleak.BleakClient("FF:EE:DD:CC:BB:AA") as client:
        print("Successfully connected")
        maxSize = client.mtu_size - 3
        print(f"MTU size is {maxSize}")
        newNrfImageFile = open(newNrfImageName, "rb").read()
        sha = hashlib.sha256(newNrfImageFile).digest() 
        await client.start_notify(smpCharectaristicUuid,cb)
        print("Succesfully started notifications")
        
        currOffset = 0
        sequence = 0

        mtuSize = min(1024, maxSize)
        while currOffset < len(newNrfImageFile):
            chunk = fullPacket(newNrfImageFile, currOffset, sequence, maxSize, sha)
            await client.write_gatt_char(smpCharectaristicUuid, chunk, response=False )
            resp = await asyncio.wait_for(responses.get(), timeout=5)
            rsp = cbor2.loads(resp[8:])
            if rsp.get("rc", 0) != 0:
                print("Issue with upload. Canceling")
                return
            currOffset = rsp["off"]
            sequence = (sequence + 1) & 0xFF
            print(f"{currOffset} B / {len(newNrfImageFile)} B")
        await confirmUpload(client)
        
            

print("Please start the transfer by pressing any key")
input()
asyncio.run(main())