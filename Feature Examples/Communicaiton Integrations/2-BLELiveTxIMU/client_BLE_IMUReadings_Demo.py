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
    - Recieve live IMU data from our SwIMU via notifications
    
The Bleak library utilizes asyncio to ensure waiting functions to not block
the execution of the main program. There are a few requirements to run a
program using asyncio in spyder including an async "main" function and nesting.
This will be notated in the code below. For more info on asyncio, see here ->
https://realpython.com/async-io-python/
"""
# Imports
import asyncio
import nest_asyncio
import time
from bleak import BleakScanner, BleakClient

# Define BLE characteristic. This is a custom generated value that each side of
# the program uses to create a communication "channel" to transmit specific 
# types of information. For more information on BLE services and
# characteristics, see this tutorial ->
# https://novelbits.io/introduction-to-bluetooth-low-energy-book/
IMU_DATA_UUID = "550e8401-e29b-41d4-a716-446655440002"
IMU_REQUEST_UUID = "550e8401-e29b-41d4-a716-446655440001"
TARGET_DEVICE = "SwIMU"

end_time = 10  # time for our data acquisition to run

# Need this to have Asyncio work in the spyder IDE
nest_asyncio.apply()

async def tx_IMU_with_read(client: BleakClient):
    ### ------------------- BLE Read Implementation ------------------ ### 
    # The BLE Read function involves a call/response between the client and
    # server, there is time assoicated with this, and therefore the band
    # width is low, however, the transmission is more reliable
        
    # Start a loop to run for 10s to read  the IMU_DATA characteristic
    start_time = time.perf_counter()
    run_time = 0
    data_list = []
    await client.write_gatt_char(IMU_REQUEST_UUID, b"START")
    while run_time < end_time:
        # Read data from the IMU characteristic
        imu_sensor_values = await client.read_gatt_char(IMU_DATA_UUID)
        # Decode bytes array into human readable string
        imu_sensor_values = imu_sensor_values.decode("utf-8") 
        # print(f"Recieved Data: {imu_sensor_values}")
        # process the data based on format "time, Ax, Ay, Az, Gx, Gy, Gz"
        split_line = imu_sensor_values.split(",")
        imu_data_line = (float(x) for x in split_line)
        data_list.append(imu_data_line)
        run_time = time.perf_counter() - start_time
    await client.write_gatt_char(IMU_REQUEST_UUID, b"END")
    print("----------------- BLE Read Implementation ---------------")    
    print(f"Number of data packets recieved in {end_time}s: {len(data_list)}")
    print(f"Realized Frequency [Hz]: {len(data_list) / run_time}")
    
async def tx_IMU_with_notify(client):
    ### ------------------ BLE Notify Implementation ----------------- ### 
    # The BLE Notify method involves setting up a callback function to
    # run whenver new data is written to a characteristic on the perephrial
    # because there is no call/reponse, there is shorter delay between
    # instances of the program running
     
    # Start a loop to run for 10s to read  the IMU_DATA characteristic
    data_list = []
    
    # define a callback function to process data when it arrives
    async def handle_IMU_notification(sender, data):            
        nonlocal data_list
        # Decode bytes array into human readable string
        imu_sensor_values = data.decode("utf-8") 
        # print(f"Recieved Data: {imu_sensor_values}")
        # process the data based on format "time, Ax, Ay, Az, Gx, Gy, Gz"
        split_line = imu_sensor_values.split(",")
        imu_data_line = (float(x) for x in split_line)
        data_list.append(imu_data_line)
        
    # Configure the notification
    await client.start_notify(IMU_DATA_UUID, handle_IMU_notification)
    
    # Write the start request
    await client.write_gatt_char(IMU_REQUEST_UUID, b"START")
    # Collect data for specified time
    await asyncio.sleep(end_time)
    # Write the end request to stop transmitting
    await client.write_gatt_char(IMU_REQUEST_UUID, b"END")

    print("----------------- BLE Notify Implementation ---------------")    
    print(f"Number of data packets recieved in {end_time}s: {len(data_list)}")
    print(f"Realized Frequency [Hz]: {len(data_list) / end_time}")
    
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
        
        # Diagnostic syntax to show the available services and characteristics
        # on the perephrial
        services = client.services
        for service in services:
            print(f"Serice: {service}")
            for char in service.characteristics:
                print(f" Characteristic: {char}")
        
        
        # We have 2 primary ways to read data from a BLE device, we can read
        # a characteristic, or setup a notification. There's advantages to
        # each. comment out one or the other to see differences in the 
        # data rate
        
        #### Comment/Uncomment each one to see the difference in behavior ###
        # await tx_IMU_with_read(client)
        await tx_IMU_with_notify(client)
        
        
if __name__ == "__main__":
    asyncio.run(main())