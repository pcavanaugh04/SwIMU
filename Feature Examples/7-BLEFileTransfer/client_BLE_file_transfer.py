# -*- coding: utf-8 -*-
"""
Welcome to the SwIMU device tutorial series. This is a series of tutorials
to get you familiar with a few facets of sports engineering, product design
and microcontroller programming. This is the third tutorial that uses a
python editor to implement Bluetooth Low-Energy (BLE) between a local computer
(client) and the SwIMU device (periphrial). Please complete the first 4 modules
in the "Hardware Programs" tutorial folder to get familiar with the functions
of the arduino device before starting this tutorial.

This makes use of the bleak library in python, which is an implementation of
the BLE protocol in python. This provides functions to configure and
communicate with BLE-endabled devices. In this tutorial we will accomplish the
following things
    - configure and connect our local device as a BLE client
    - request and recieve a file from a BLE server
    
The Bleak library utilizes asyncio to ensure waiting functions to not block
the execution of the main program. There are a few requirements to run a
program using asyncio in spyder including an async "main" function and nesting.
This will be notated in the code below. For more info on asyncio, see here ->
https://realpython.com/async-io-python/
"""

import asyncio
import nest_asyncio
import time
import os
from datetime import datetime
from bleak import BleakScanner, BleakClient

FILE_TX_REQUEST_UUID = "550e8403-e29b-41d4-a716-446655440001"
FILE_TX_UUID = "550e8403-e29b-41d4-a716-446655440002"
FILE_TX_COMPLETE_UUID = "550e8403-e29b-41d4-a716-446655440003"
FILE_NAME_UUID = "550e8403-e29b-41d4-a716-446655440004"

nest_asyncio.apply()

async def main():
    devices = await BleakScanner.discover(timeout=5)
    address = ""
    for device in devices:
        print(device)
        if "SwIMU" in device.__str__():
            address = device.__str__().split(": ")[0]

    if not address:
        print("Target Device Not Found!")
        return
    
    # Context manager to configure device
    async with BleakClient(address, timeout=20) as client:
        print("Device Connected for File Transfer!")
        
        # Send Request for File transfer
        await client.write_gatt_char(FILE_TX_REQUEST_UUID, b"SEND_FILE")
        # Wait for server to send file name
        file_name = await client.read_gatt_char(FILE_NAME_UUID)
        file_name = file_name.decode("utf-8")
        if (file_name == "ERROR"):
            print("Error on Server accessing file!")
            return
        print(f"Recieved file name: {file_name}")
        
        # setup variable to recieve file data
        file_data = b""      
        
        # Callback to accumulate packets of file data from server
        async def handle_file_data(sender, data):
            start_time = time.perf_counter()
            nonlocal file_data
            file_data += data
        
        # Setup the notificaiton handler
        await client.start_notify(FILE_TX_UUID, handle_file_data)
    
        # Initialize a Future event to hold until file transfer is complete
        transfer_complete = asyncio.Future()
        
        # Callback to handle completion notificaiton
        def handle_transfer_complete(sender, data):
            if data.decode("utf-8") == "TRANSFER_COMPLETE":
                transfer_complete.set_result(True)
                print("Transfer Complete")
                
        # config file complete notificaiton manager
        await client.start_notify(FILE_TX_COMPLETE_UUID, handle_transfer_complete)
        # Acknowledge file name to indicate we're ready to recieve file data 
        await client.write_gatt_char(FILE_TX_REQUEST_UUID, b"START")
        print("Client Acknowledged File name. Beginning Transfer")
        file_tx_start = time.perf_counter()

        # Wait for transfer compltete notification
        await transfer_complete
        print(f"File tx in {time.perf_counter() - file_tx_start} s")

        
    # Write recieved contents to file
    save_path = os.path.join(r"C:\Users\patri\Downloads", file_name.split('/')[-1])
    with open(save_path, "wb") as f:
        f.write(file_data)
    
    print(f"File saved locally to {save_path}.")
        
if __name__ == "__main__":
    asyncio.run(main())