import bleak
import asyncio
import os

async def main():
    async with BleakClient("FF:EE:DD:CC:BB:AA") as client:
        # TODO: Get file and post it to the SMP service
        # SMP service UUID: 8D53DC1D-1DB7-4CD3-868B-8A527460AA84 
    

asyncio.run(main())
