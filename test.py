import bleak
import asyncio
import os
serviceUuid = "8D53DC1D-1DB7-4CD3-868B-8A527460AA84"
newNrfImageName = "zephyr.signed.bin"
currWorkingDirectory = os.getcwd()
newNrfImageWithPath = (currWorkingDirectory+'\\'+newNrfImageName)
flagsToBeUsed = os.O_BINARY|os.O_RDONLY
newNrfImageFile = os.open(newNrfImageWithPath, flags=flagsToBeUsed)
for x in newNrfImageFile:
    print(x)
    break
"""async def main():
    async with BleakClient("FF:EE:DD:CC:BB:AA") as client:
        chunk = b'0x00'
        client.write(serviceUuid, chunk, response=True)
        # TODO: Get file and post it to the SMP service
        # SMP service UUID: 8D53DC1D-1DB7-4CD3-868B-8A527460AA84 
    

asyncio.run(main())"""
#print(newNrfImageFile)