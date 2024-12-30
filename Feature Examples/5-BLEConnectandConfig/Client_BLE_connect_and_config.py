# -*- coding: utf-8 -*-
"""
Welcome to the SwIMU device tutorial series. This is a series of tutorials
to get you familiar with a few facets of sports engineering, product design
and microcontroller programming. This is the first tutorial that uses a
python editor to implement Bluetooth Low-Energy (BLE) between a local computer
(client) and the SwIMU device (periphrial). Please complete the first 4 modules
in the "Hardware Programs" tutorial folder to get familiar with the functions
of the arduino device before starting this tutorial.

This makes use of the bleak library in python, which is an implementation of
the BLE protocol in python. This provides functions to configure and
communicate with BLE-endabled devices. In this tutorial we will accomplish the
following things
    - Configure our local computer as a BLE client
    - Scan for BLE Devices in our area
    - Connect to a BLE-enabled periphrial (SwIMU device)
    - Write data to characteristics on the server to configure variables
    
The Bleak library utilizes asyncio to ensure waiting functions to not block
the execution of the main program. There are a few requirements to run a
program using asyncio in spyder including an async "main" function and nesting.
This will be notated in the code below. For more info on asyncio, see here ->
https://realpython.com/async-io-python/
"""

# Bluetooth LE scanner
# Prints the name and address of every nearby Bluetooth LE device

import asyncio
import nest_asyncio
import time
from datetime import datetime
from bleak import BleakScanner, BleakClient

# Define BLE characteristic. This is a custom generated value that each side of
# the program uses to create a communication "channel" to transmit specific 
# types of information. For more information on BLE services and
# characteristics, see this tutorial ->
# https://novelbits.io/introduction-to-bluetooth-low-energy-book/
DATETIME_UUID = "550e8401-e29b-41d4-a716-446655440001"
PERSONNAME_UUID = "550e8401-e29b-41d4-a716-446655440002"
ACTIVITY_TYPE_UUID = "550e8401-e29b-41d4-a716-446655440003"
FILE_NAME_UUID = "550e8401-e29b-41d4-a716-446655440004"
DT_FMT = "%Y_%m_%d_%H_%M_%S"
TARGET_DEVICE = "SwIMU"


# Need this to have Asyncio work in the spyder IDE
nest_asyncio.apply()

async def user_input(input_msg: str) -> str:
    input_value = input(input_msg)
    return input_value
    

# Define the "main" function for Asyncio. 
async def main():
    # Scan for ble devices in our proximity
    devices = await BleakScanner.discover(timeout=5)
    address = ""
    # When scanning is complete, see if our target device name was found
    for device in devices:
        print(device)
        # If so, extract the BLE address associated with the device. these
        # adresses are unique to each device and is how we connect/
        # communicate with them
        if TARGET_DEVICE in device.__str__():
            address = device.__str__().split(": ")[0]

    if not address:
        print("Target Device Not Found!")
        return
    
    # Use the found address to connect to the device
    async with BleakClient(address, timeout=20) as client:
        print("Device Connected!")
        
        # poll the user with an asyncio-safe function to prevent blocking
        input_name = await user_input("Enter the SwIMU user's name: ")
        # Write to swimmer name characteristic
        await client.write_gatt_char(PERSONNAME_UUID, input_name.encode("utf-8"))
        # Update the datetime characterisitc to ensure an accurate refrence value
        datetime_str = datetime.now().strftime(DT_FMT)
        await client.write_gatt_char(DATETIME_UUID, datetime_str.encode("utf-8"))
        # Repeat with the activity 
        input_activity = await user_input("Enter the SwIMU activty name: ")
        await client.write_gatt_char(ACTIVITY_TYPE_UUID, input_activity.encode("utf-8"))
        datetime_str = datetime.now().strftime(DT_FMT)
        await client.write_gatt_char(DATETIME_UUID, datetime_str.encode("utf-8"))
        
        # Get the new filename from the server after sending our config information
        new_file_name = await client.read_gatt_char(FILE_NAME_UUID)
        print(f"Configured file name: {new_file_name.decode('utf-8')}")
        
if __name__ == "__main__":
    asyncio.run(main())