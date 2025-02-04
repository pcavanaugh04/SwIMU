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
from datetime import datetime
import time
import os
from bleak import BleakScanner, BleakClient

FILE_TX_SERVICE_UUID = "550e8404-e29b-41d4-a716-446655440000"
FILE_TX_REQUEST_UUID = "550e8405-e29b-41d4-a716-446655440001"
FILE_TX_UUID = "550e8405-e29b-41d4-a716-446655440002"
FILE_TX_COMPLETE_UUID = "550e8405-e29b-41d4-a716-446655440003"
FILE_NAME_UUID = "550e8405-e29b-41d4-a716-446655440004"

CONFIG_SERVICE_UUID = "550e8400-e29b-41d4-a716-446655440000"
DATETIME_UUID = "550e8401-e29b-41d4-a716-446655440001"
PERSONNAME_UUID = "550e8401-e29b-41d4-a716-446655440002"
ACTIVITY_TYPE_UUID = "550e8401-e29b-41d4-a716-446655440003"
FILE_NAME_UUID = "550e8401-e29b-41d4-a716-446655440004"

IMU_TX_SERVICE_UUID = "550e8402-e29b-41d4-a716-446655440000"
IMU_REQUEST_UUID = "550e8403-e29b-41d4-a716-446655440001"
IMU_DATA_UUID = "550e8403-e29b-41d4-a716-446655440002"

DT_FMT = "%Y_%m_%d_%H_%M_%S"
TARGET_DEVICE = "SwIMU"

nest_asyncio.apply()

class BLEClient(BleakClient):
    def __init__(self, address, timeout=10):
        # self.client = BleakClient(address, timeout=timeout)
        self.connected = False
        super().__init__(address, timeout=timeout)

    async def handle_disconnect(self, client):
        print("Disconnected from server!")
        self.connected = False
        # Optionally trigger a reconnection attempt here or wait for a server event.
        
    async def handle_connect(self, client):
        print("Connected to Server!")
        

    # async def connect(self):
    #     try:
    #         print("Attempting to connect...")
    #         # self.client.set_disconnected_callback(self.handle_disconnect)
    #         await self.client.connect()
    #     except Exception as e:
    #         print(f"Failed to connect: {e}")
    #         self.connected = False
    #     else:
    #         self.connected = True
    #         print(f"Connected to server: {self.address}")
    #         # await self.handle_modes()

    # async def disconnect(self):
    #     if self.connected:
    #         print("Disconnecting...")
    #         await self.client.disconnect()
    #         self.connected = False

    async def monitor(self):
        while True:
            if not self.connected:
                print("Waiting for server to reconnect...")
                await self.connect()
            else:
                print("Client is connected.")
            await asyncio.sleep(5)  # Adjust the polling frequency as needed
            
    async def config_device(self):
        # poll the user with an asyncio-safe function to prevent blocking
        input_name = await user_input("Enter the SwIMU user's name: ")
        # Write to swimmer name characteristic
        await self.write_gatt_char(PERSONNAME_UUID, input_name.encode("utf-8"))
        # Update the datetime characterisitc to ensure an accurate refrence value
        datetime_str = datetime.now().strftime(DT_FMT)
        await self.write_gatt_char(DATETIME_UUID, datetime_str.encode("utf-8"))
        # Repeat with the activity 
        input_activity = await user_input("Enter the SwIMU activty name: ")
        await self.write_gatt_char(ACTIVITY_TYPE_UUID, input_activity.encode("utf-8"))
        datetime_str = datetime.now().strftime(DT_FMT)
        await self.write_gatt_char(DATETIME_UUID, datetime_str.encode("utf-8"))
        
        # Get the new filename from the server after sending our config information
        new_file_name = await self.read_gatt_char(FILE_NAME_UUID)
        print(f"Configured file name: {new_file_name.decode('utf-8')}")
        
    async def rx_IMU_readings_mode(self):
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
            print(f"Recieved Data: {imu_sensor_values}")
            # process the data based on format "time, Ax, Ay, Az, Gx, Gy, Gz"
            split_line = imu_sensor_values.split(",")
            imu_data_line = (float(x) for x in split_line)
            data_list.append(imu_data_line)
            
        # Configure the notification
        await self.start_notify(IMU_DATA_UUID, handle_IMU_notification)
        await user_input("Press Enter to Start Data Recording")
        # Write the start request
        await self.write_gatt_char(IMU_REQUEST_UUID, b"START")
        start_time = time.perf_counter()
        # Collect data for specified time
        await user_input("Press Enter to Stop Data Recording")
        # Write the end request to stop transmitting
        await self.write_gatt_char(IMU_REQUEST_UUID, b"END")
        record_time = time.perf_counter() - start_time

        print("----------------- BLE Notify Implementation ---------------")    
        print(f"Number of data packets recieved in {record_time}s: {len(data_list)}")
        print(f"Realized Frequency [Hz]: {len(data_list) / record_time}")
        
    async def handle_modes(self):
        if self.connected:
            # Check for user input and perform accordingly
            print("\nSelect Mode:")
            print("\t1. Config Mode")
            print("\t2. IMU Live Tx Mode")
            print("\t3. File Tx Mode")
            print("\t4. Disconnect")
            choice = input("Enter choice (1-4): ")
            
            if choice == '1':
                await self.config_device()
            elif choice == '2':
                await self.rx_IMU_readings_mode()
            elif choice == '3':
                await self.file_tx_mode()
            elif choice == '4':
                print("Disconnecting...")
                await self.disconnect()
                return
            else:
                print("Invalid choice. Try again.")
        else: 
            # Try to connect
            await self.connect()

                
    async def file_tx_mode(self):
        pass


async def handle_input():
    print("\nSelect an Command:")
    print("\t1. Connect")
    print("\t2. Disconnect")
    choice = input("Enter choice (1-2): ")
    
    match choice:
        case '1':
            return "connect"
        
        case '2':
            return "disconnect"    

    

async def user_input(input_msg: str) -> str:
    input_value = input(input_msg)
    return input_value

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
        print(f"Recieved Data: {imu_sensor_values}")
        # process the data based on format "time, Ax, Ay, Az, Gx, Gy, Gz"
        split_line = imu_sensor_values.split(",")
        imu_data_line = (float(x) for x in split_line)
        data_list.append(imu_data_line)
        
    # Configure the notification
    await client.start_notify(IMU_DATA_UUID, handle_IMU_notification)
    await user_input("Press Enter to Start Data Recording")
    # Write the start request
    await client.write_gatt_char(IMU_REQUEST_UUID, b"START")
    start_time = time.perf_counter()
    # Collect data for specified time
    await user_input("Press Enter to Stop Data Recording")
    # Write the end request to stop transmitting
    await client.write_gatt_char(IMU_REQUEST_UUID, b"END")
    record_time = time.perf_counter() - start_time

    print("----------------- BLE Notify Implementation ---------------")    
    print(f"Number of data packets recieved in {record_time}s: {len(data_list)}")
    print(f"Realized Frequency [Hz]: {len(data_list) / record_time}")
    
async def scan(target_device_name: str):
    # Scan for ble devices in our proximity
    devices = await BleakScanner.discover(timeout=5, return_adv=True)
    # print(devices)
    address = ""
    # When scanning is complete, see if our target device name was found
    for device_addr, device_info in devices.items():
        device = device_info[0]
        print(f"{device_addr, device.name}")
        # If so, extract the BLE address associated with the device. these
        # adresses are unique to each device and is how we connect/
        # communicate with them
        if (device.name is not None) and (target_device_name in device.name):
            device_metadata = device_info[1]
            print(f"Target Device Metadata: {device_metadata}")
            return device_info

    if not address:
        print("Target Device Not Found!")
        return None

# Define the "main" function for Asyncio. 
async def main():
    # Scan for ble devices in our proximity    
    
    # client = BLEClient(address, timeout=20)

    while True:
        user_input = await handle_input()
        if "connect" in user_input:
            # Device is a tuple containing (BLEDevice, AdvData) from the scanner
            device = await scan(TARGET_DEVICE)
            if device is None:
                print("Failed to discover device! Resetting...")
                continue
            
            adv_service = device[1].service_uuids[0]
            address = device[0].address
            
            print(f"Connecting to address: {address}")

            async with BLEClient(address, timeout=20) as client:
                print("Device Connected!")
                if CONFIG_SERVICE_UUID in adv_service:
                    await client.config_device()
                elif IMU_TX_SERVICE_UUID in adv_service:
                    await client.rx_IMU_readings_mode()
                        
                # Read contents of the advertising packet
        
        
    
    # # initialize Client device with discovered address
    # try:
    #     client.monitor_task = asyncio.create_task(client.monitor())
    #     while(True):
    #         await client.handle_modes()
    #         await asyncio.sleep(5)
    #     # await client.connect()
    #     # Start monitoring
    # except KeyboardInterrupt:
    #     print("Interrupted by user.")
    # finally:
    #     if client.monitor_task:
    #         client.monitor_task.cancel()
            
    #     await client.disconnect()
        
    #     print("Cleaned up and exiting.")
        
r"""
---------------------Reused code from file tx script -----------------------
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
    
"""
        
if __name__ == "__main__":
    asyncio.run(main())