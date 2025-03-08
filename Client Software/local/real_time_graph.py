# file to connect, recieve and visualize IMU from a BLE periphrial
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
import sys
import numpy as np
from PyQt5 import QtWidgets, QtCore, uic
from PyQt5.QtWidgets import QMainWindow, QApplication, QWidget, QVBoxLayout, QPushButton, QLabel
from PyQt5.QtCore import QObject, QThread, pyqtSignal, pyqtSlot
import pyqtgraph as pg


FILE_TX_SERVICE_UUID = "550e8404-e29b-41d4-a716-446655440000"
FILE_TX_REQUEST_UUID = "550e8405-e29b-41d4-a716-446655440001"
FILE_TX_UUID = "550e8405-e29b-41d4-a716-446655440002"
FILE_TX_COMPLETE_UUID = "550e8405-e29b-41d4-a716-446655440003"
FILE_TX_NAME_UUID = "550e8405-e29b-41d4-a716-446655440004"

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

# Define the BLEClient class to handle the connection and communication with the
# BLE periphrial. This class will inherit from the BleakClient class which is
# provided by the bleak library. This class will have the following methods:
#     - connect: to establish a connection with the periphrial
#     - disconnect: to close the connection with the periphrial
#     - monitor: to check the connection status and attempt to reconnect if
#       disconnected
#     - config_device: to configure the periphrial with user information
#     - rx_IMU_readings_mode: to recieve IMU data from the periphrial
#     - file_rx_mode: to recieve a file from the periphrial
#     - write_to_file: to write the recieved file data to a file on the local
#       machine

# This will be hosted in a QT thread to run in the background of the main program. When the user prompts connect,
# the program will spin up a qt thread. If it's in live tx mode, it will graph the incoming data in real time

class BLEClient(BleakClient, QObject):
    new_data = pyqtSignal(object)

    def __init__(self, address, timeout=10):
        self.connected = False
        self.file_rx_setup_flag = False
        QObject.__init__(self, parent=None)
        print("QObject initilzied in BLEClient")
        # super(BleakClient, self).__init__(address, timeout=timeout)
        BleakClient.__init__(self, address, timeout=timeout)
        print("BleakClient initilzied in BLEClient")

        file_data = b""
                # Start a loop to run for 10s to read  the IMU_DATA characteristic
        self.times = []
        self.Ax = []
        self.Ay = []
        self.Az = []
        self.Gx = []
        self.Gy = []
        self.Gz = []
        self.sensor_list = [self.times, self.Ax, self.Ay, self.Az, self.Gx, self.Gy, self.Gz]

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
        datetime_str = datetime.now().strftime(DT_FMT)
        await self.write_gatt_char(DATETIME_UUID, datetime_str.encode("utf-8"))
        await self.write_gatt_char(ACTIVITY_TYPE_UUID, input_activity.encode("utf-8"))
        # Get the new filename from the server after sending our config information
        # new_file_name = await self.read_gatt_char(FILE_NAME_UUID)
        # print(f"Configured file name: {new_file_name.decode('utf-8')}")
        
    async def rx_IMU_readings_mode(self):
        ### ------------------ BLE Notify Implementation ----------------- ### 
        # The BLE Notify method involves setting up a callback function to
        # run whenver new data is written to a characteristic on the perephrial
        # because there is no call/reponse, there is shorter delay between
        # instances of the program running
         
        # Start a loop to run for 10s to read  the IMU_DATA characteristic
        # define a callback function to process data when it arrives
        async def handle_IMU_notification(sender, data):            
            # Decode bytes array into human readable string
            imu_sensor_values = data.decode("utf-8") 
            # print(f"Recieved Data: {imu_sensor_values}")
            # process the data based on format "time, Ax, Ay, Az, Gx, Gy, Gz"
            split_line = imu_sensor_values.split(",")
            imu_data_line = [float(x) for x in split_line]
            # for i, value in enumerate(imu_data_line):
            #     self.sensor_list[i].append(value)
            self.new_data.emit(imu_data_line)
            # print(f"Sensor List: {self.sensor_list}")
        # Configure the notification
        await self.start_notify(IMU_DATA_UUID, handle_IMU_notification)
        await user_input("Press Enter to Start Data Transmission")
        # Write the start request
        await self.write_gatt_char(IMU_REQUEST_UUID, b"START")
        start_time = time.perf_counter()
        self.tx_active = True
        while self.tx_active:
            await asyncio.sleep(0.1)

    async def start_IMU_readings(self):
        await self.write_gatt_char(IMU_REQUEST_UUID, b"START")

    async def stop_IMU_readings(self):
        await self.write_gatt_char(IMU_REQUEST_UUID, b"END")
        self.tx_active = False
        """
        record_time = time.perf_counter() - start_time

        print("----------------- BLE Notify Implementation ---------------")    
        print(f"Number of data packets recieved in {record_time}s: {len(data_list)}")
        print(f"Realized Frequency [Hz]: {len(data_list) / record_time}")
"""
        
    
    async def write_to_file(self, save_path, file_data):
        with open(save_path, "wb") as f:
            f.write(file_data)
        print(f"Recieved Data Written to file: {save_path}")

                
    async def file_rx_mode(self):

        # Send Request for File transfer
        await user_input("Press Enter to Start File Transfer")

        await self.write_gatt_char(FILE_TX_REQUEST_UUID, b"SEND_FILES")
        status = await self.read_gatt_char(FILE_TX_REQUEST_UUID)
        status = status.decode("utf-8")
        if (status == "READY"):
            print("Periphrial Ready to Transmit Files")
            first_file_flag = True

        else:
            print("Unhandled error case. Potentially no files available. Canceling transfer")
            return   
        
        # define and assign notification callbacks on first file only
        if not self.file_rx_setup_flag:
            
            file_data = b""
            # Callback to accumulate packets of file data from server
            async def handle_file_data(sender, data):
                if data:
                    nonlocal file_data
                    # packet = data.decode("utf-8")
                    file_data += data
                    print(f"Written to file data variable: {data}")
                else:
                    print("Received empty data packet!")
               
            # Setup the notificaiton handler
            await self.start_notify(FILE_TX_UUID, handle_file_data)
            
                        # Callback to handle completion notificaiton
            def handle_transfer_complete(sender, data):
                if data.decode("utf-8") == "TRANSFER_COMPLETE":
                    transfer_complete.set_result(True)
                    print("Transfer Complete")
            
            # config file complete notificaiton manager
            await self.start_notify(FILE_TX_COMPLETE_UUID, handle_transfer_complete)
            self.file_rx_setup_flag = True

        while True:
            # Wait for server to send file name
            file_name = await self.read_gatt_char(FILE_TX_NAME_UUID)
            # Write an acknowledgement
            await self.write_gatt_char(FILE_TX_NAME_UUID, b"ACK")

            file_name = file_name.decode("utf-8")
            if (file_name == "ERROR"):
                print("Error on Server accessing file!")
                return
            else:
                print(f"Recieved file name: {file_name}")
            
            # setup variable to recieve file data
            file_data = b""      

            # Initialize a Future event to hold until file transfer is complete
            transfer_complete = asyncio.Future()
            
            # Acknowledge file name to indicate we're ready to recieve file data 
            await self.write_gatt_char(FILE_TX_REQUEST_UUID, b"START")
            print("Client Acknowledged File name. Beginning Transfer")
            file_tx_start = time.perf_counter()
        
            # Wait for transfer compltete notification
            await transfer_complete
            print(f"File tx in {time.perf_counter() - file_tx_start} s")
        
            
            # Write recieved contents to file
            save_path = os.path.join(r"C:\Users\patri\Downloads", file_name)
            await self.write_to_file(save_path, file_data)
            
            # Query periphrial for more files.
            

            await self.write_gatt_char(FILE_TX_REQUEST_UUID, b"MORE_FILES?")
            status = await self.read_gatt_char(FILE_TX_REQUEST_UUID)
            status = status.decode("utf-8")
            if (status == "MORE_FILES"):
                print("Ready to recieve another file.")
                file_data = b""
                
            elif (status == "DONE"):
                print("All files transmitted!")
                break
            

async def prompt_connection():
    input("Press Enter to Initiate Connection:")

async def user_input(input_msg: str) -> str:
    input_value = input(input_msg)
    return input_value
    
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
async def run():
    # Scan for ble devices in our proximity    
    
    # client = BLEClient(address, timeout=20)

    # while True:
        # await prompt_connection()
    # Device is a tuple containing (BLEDevice, AdvData) from the scanner
    device = await scan(TARGET_DEVICE)
    if device is None:
        print("Failed to discover device! Resetting...")
    
    adv_service = device[1].service_uuids[0]
    address = device[0].address
    
    print(f"Connecting to address: {address}")

    async with BLEClient(address, timeout=20) as client:
        print("Device Connected!")
        if CONFIG_SERVICE_UUID in adv_service:
            await client.config_device()
        elif IMU_TX_SERVICE_UUID in adv_service:
            await client.rx_IMU_readings_mode()
        
        elif FILE_TX_SERVICE_UUID in adv_service:
            await client.file_rx_mode()

class BLEWorker(QObject):
    finished = pyqtSignal()
    connected = pyqtSignal(bool)

    def __init__(self):
        super().__init__()
        self._is_running = True
        self.loop = None

    @pyqtSlot()
    def run(self):
        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)
        try:
            self.loop.run_until_complete(self.main_BLE_client())
        except Exception as e:
            print(f"Error in BLE Worker: {e}")

        finally:
            self.loop.close()
            self.finished.emit()

    async def main_BLE_client(self):
        # Scan for ble devices in our proximity    
    
        device = await scan(TARGET_DEVICE)
        if device is None:
            print("Failed to discover device! Resetting...")
            return
        
        adv_service = device[1].service_uuids[0]
        address = device[0].address
        
        print(f"Connecting to address: {address}")

        async with BLEClient(address, timeout=20) as client:
            self.client = client
            self.connected.emit(True)
            print("Device Connected!")
            if CONFIG_SERVICE_UUID in adv_service:
                await self.client.config_device()
            elif IMU_TX_SERVICE_UUID in adv_service:
                await self.client.rx_IMU_readings_mode()
            
            elif FILE_TX_SERVICE_UUID in adv_service:
                await self.client.file_rx_mode()
            self.connected.emit(False)
    def stop(self):
        """
        Stop the BLE loop by setting the running flag to False.
        """
        self._is_running = False
        # If the asyncio loop is running, schedule it to stop.
        if self.loop:
            self.loop.call_soon_threadsafe(self.loop.stop)
                        

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.init_ui()
        
        # Data buffers (replace these with your BLE data lists)
        self.max_points = 300  # maximum points to display
        
        # Timer to update plots
        self.timer = QtCore.QTimer()
        self.timer.setInterval(50)  # update every 50 ms
        self.timer.timeout.connect(self.update_plots)
        # self.timer.start()
        self.graph_slices = [[], [], [], [], [], [], []]
        
        # initialize a client attribute, update when BLEWorker emits connected signal
        self.client = None

        # mode flags
        self.in_data_tx_mode = False
        self.in_device_config_mode = False
        self.in_file_tx_mode = False
        
    def init_ui(self):
        # Initilize elements of layout and create connections to slots and signals

        # layout = QtWidgets.QVBoxLayout()
        import os
        ui_path = os.path.join(os.getcwd(), "SwIMU_client_UI.ui")
        uic.loadUi(ui_path, self)

        # Accelerometer Plot
        self.accel_plot = pg.PlotWidget(title="Accelerometer Data")
        self.accel_plot.addLegend()
        self.accel_plot.setLabel('left', "Acceleration (g)")
        self.accel_plot.setLabel('bottom', "Samples")
        self.accel_x_curve = self.accel_plot.plot(pen='r', name='Accel X')
        self.accel_y_curve = self.accel_plot.plot(pen='g', name='Accel Y')
        self.accel_z_curve = self.accel_plot.plot(pen='b', name='Accel Z')
        
        # Gyroscope Plot
        self.gyro_plot = pg.PlotWidget(title="Gyroscope Data")
        self.gyro_plot.addLegend()
        self.gyro_plot.setLabel('left', "Angular Velocity (°/s)")
        self.gyro_plot.setLabel('bottom', "Samples")
        self.gyro_x_curve = self.gyro_plot.plot(pen='r', name='Gyro X')
        self.gyro_y_curve = self.gyro_plot.plot(pen='g', name='Gyro Y')
        self.gyro_z_curve = self.gyro_plot.plot(pen='b', name='Gyro Z')

        # Connect Button
        # Setup button UI to connect with BLE device
        self.connect_button.clicked.connect(self.run_BLE_worker)
        self.data_tx_button.clicked.connect(self.start_stop_data_tx)
        self.file_tx_button.clicked.connect(self.start_stop_file_tx)
        self.config_field.returnPressed.connect(self.send_config_data)

    @pyqtSlot()
    def start_stop_data_tx(self):
        # Start Data Transmission. This method will be inaccessible until the client is connected,
        # so no need to check for client status
        if not self.in_data_tx_mode:
            # send peripheral the start command
            self.client.start_IMU_readings()
            self.data_tx_button.setText("Stop Data Tx")
            self.in_data_tx_mode = True

        else:
            self.client.stop_IMU_readings()
            self.data_tx_button.setText("Stop Data Tx")
            self.in_data_tx_mode = False

    @pyqtSlot()
    def start_stop_file_tx(self):
        # Start Data Transmission. This method will be inaccessible until the client is connected,
        # so no need to check for client status
        if not self.in_data_tx_mode:
            # send peripheral the start command
            self.client.start_IMU_readings()
            self.data_tx_button.setText("Stop Data Tx")
            self.in_data_tx_mode = True

        else:
            self.client.stop_IMU_readings()
            self.data_tx_button.setText("Stop Data Tx")
            self.in_data_tx_mode = False

    @pyqtSlot()
    def send_config_data(self):
        # Get line of text from config field
        config_data = self.config_field.text()
        # Send the data to the peripheral
        pass

    
    def run_BLE_worker(self):
        self.worker = BLEWorker()
        self.worker.connected.connect(self.update_connection_status)
        self.thread = QThread()
        self.worker.moveToThread(self.thread)
        self.thread.started.connect(self.worker.run)
        self.worker.finished.connect(self.thread.quit)
        self.worker.finished.connect(self.worker.deleteLater)
        self.thread.finished.connect(self.thread.deleteLater)
        self.thread.start()
    
    def update_connection_status(self, connected):
        if connected:
            self.connect_button.setText("Disconnect")
            self.client = self.worker.client
            self.client.new_data.connect(self.update_data)
            self.timer.start()

        else:
            self.connect_button.setText("Connect")
            self.timer.stop()
            self.client = None
            self.graph_data = None
            
    def update_data(self, data):
        # print(f"Data Recieved: {data}")
        for i, value in enumerate(data):
            self.graph_slices[i].append(value)
        
    def update_plots(self):
        # Simulate new data values here.
        # In your application, update self.accel_data and self.gyro_data with new readings from your BLE backend.
        
        # Limit data to the most recent max_points
        for i, data_list in enumerate(self.graph_slices):
            # print(f"Data List: {data_list}")
            if len(data_list) > self.max_points:
                self.graph_slices[i] = data_list[-self.max_points:]
            else:
                self.graph_slices[i] = data_list
        # Generate x-axis values (sample indices)
        x_axis = self.graph_slices[0]
        # print(f"X Axis: {x_axis}")
        # Update accelerometer curves
        self.accel_x_curve.setData(x_axis, self.graph_slices[1])
        self.accel_y_curve.setData(x_axis, self.graph_slices[2])
        self.accel_z_curve.setData(x_axis, self.graph_slices[3])
        
        # Update gyroscope curves
        self.gyro_x_curve.setData(x_axis, self.graph_slices[4])
        self.gyro_y_curve.setData(x_axis, self.graph_slices[5])
        self.gyro_z_curve.setData(x_axis, self.graph_slices[6])

if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    # window.resize(800, 600)
    window.setWindowTitle("BLE Accelerometer and Gyro Visualizer")
    window.show()
    sys.exit(app.exec_())