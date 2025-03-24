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
import sys
from SwIMU_BLE import BLEWorker
from PyQt5 import QtWidgets, QtCore, uic
from PyQt5.QtWidgets import QMainWindow
from PyQt5.QtCore import QObject, QThread, pyqtSignal, pyqtSlot


class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.init_ui()
        
        # Data buffers (replace these with your BLE data lists)
        self.max_points = 600  # maximum points to display
        
        # Timer to update plots
        self.graph_update_timer = QtCore.QTimer()
        self.graph_update_timer.setInterval(50)  # update every 50 ms
        self.graph_update_timer.timeout.connect(self.update_plots)
        # self.timer.start()
        self.graph_slices = [[], [], [], [], [], [], []]
        
        # initialize a client attribute, update when BLEWorker emits connected signal
        self.client = None

        # mode flags
        self.in_data_tx_mode = False
        self.in_device_config_mode = False
        self.in_file_tx_mode = False
        self.config_fields = ['Name', 'Activity']
        self.config_info = {}
        self.config_field_index = 0
        
    def init_ui(self):
        # Initilize elements of layout and create connections to slots and signals

        # layout = QtWidgets.QVBoxLayout()
        import os
        ui_path = os.path.join(os.getcwd(), "SwIMU_client_UI.ui")
        uic.loadUi(ui_path, self)

        # Accelerometer Plot
        # self.accel_plot = pg.PlotWidget(title="Accelerometer Data")
        self.accel_plot.addLegend()
        self.accel_plot.setLabel('left', "Acceleration (g)")
        self.accel_plot.setLabel('bottom', "Time [s]")
        self.accel_x_curve = self.accel_plot.plot(pen='r', name='Accel X')
        self.accel_y_curve = self.accel_plot.plot(pen='g', name='Accel Y')
        self.accel_z_curve = self.accel_plot.plot(pen='b', name='Accel Z')
        
        # Gyroscope Plot
        # self.gyro_plot = pg.PlotWidget(title="Gyroscope Data")
        self.gyro_plot.addLegend()
        self.gyro_plot.setLabel('left', "Angular Velocity (°/s)")
        self.gyro_plot.setLabel('bottom', "Time [s]")
        self.gyro_x_curve = self.gyro_plot.plot(pen='r', name='Gyro X')
        self.gyro_y_curve = self.gyro_plot.plot(pen='g', name='Gyro Y')
        self.gyro_z_curve = self.gyro_plot.plot(pen='b', name='Gyro Z')

        # Connect Button
        # Setup button UI to connect with BLE device
        self.connect_button.clicked.connect(self.run_BLE_worker)
        self.data_tx_button.clicked.connect(self.start_stop_data_tx)
        self.file_tx_button.clicked.connect(self.start_stop_file_tx)
        self.config_input_field.returnPressed.connect(self.collect_config_data)

    @pyqtSlot()
    def start_stop_data_tx(self):
        # Start Data Transmission. This method will be inaccessible until the client is connected,
        # so no need to check for client status
        self.worker.set_data_tx_status(not self.in_data_tx_mode)
        if not self.in_data_tx_mode:
            # self.accel_plot.clear()
            # self.gyro_plot.clear()
            # send peripheral the start command
            # Clear Graph and Reset Axis to zero
            # self.graph_data = None
            self.graph_update_timer.start()   
            self.data_tx_button.setText("Stop Data Tx")
            self.in_data_tx_mode = True

        else:
            self.graph_update_timer.stop()
            self.data_tx_button.setText("Start Data Tx")
            self.in_data_tx_mode = False
            self.worker.connected.emit('')

    @pyqtSlot()
    def start_stop_file_tx(self):
        # Start Data Transmission. This method will be inaccessible until the client is connected,
        # so no need to check for client status
        if not self.in_file_tx_mode:
            # send peripheral the start command
            self.data_tx_button.setText("Stop File Tx")
            self.in_file_tx_mode = True

        else:
            self.file_tx_button.setText("Start File Tx")
            self.in_file_tx_mode = False

    @pyqtSlot()
    def collect_config_data(self):
        # Get line of text from config field
        if self.config_mode:    
            config_text = self.config_input_field.text()
            
            self.config_info[self.config_fields[self.config_field_index]] = config_text 
            
            self.config_field_index += 1
            
            self.update_config_field_label()
            print(f"Contents of config dict: {self.config_info}")
            # If all data has been entered, update client property and clean up process
            if self.config_field_index >= len(self.config_fields):
                self.worker.set_config_attribute(self.config_info)
                self.config_field_index = 0
                print("Writing to config engries attirbute from Main window")


    def update_config_field_label(self):
        self.config_input_field.clear()
        
        if self.config_field_index >= len(self.config_fields):
            self.config_field_label.setText("Connect to Configure Device")
            return
        
        if self.config_mode:
            self.config_field_label.setText(f"Enter the User's {self.config_fields[self.config_field_index]}:")
            
    
    def run_BLE_worker(self):
        self.worker = BLEWorker()
        self.worker.connected.connect(self.update_connection_status)
        self.worker.finished.connect(self.worker.deleteLater)
        self.worker.start()
    
    def update_connection_status(self, connection_state):
        print(f"Recieved connection update: {connection_state}")
        
        if len(connection_state) > 0:
            self.client = self.worker.client
            self.connect_button.setText("Disconnect")

            if 'config' in connection_state:
                # self.in_device_config_mode = True
                self.config_input_frame.setEnabled(True)
                self.config_mode = True
                self.update_config_field_label()

            elif 'data_tx' in connection_state:
                # self.in_data_tx_mode = True
                self.client.new_data.connect(self.update_data)
                self.data_tx_button.setEnabled(True)

            elif 'file_tx' in connection_state:
                # self.in_file_tx_mode = True
                self.file_tx_button.setEnabled(True)

        else:
            # Disable buttons and reset UI elements
            self.connect_button.setText("Connect")
            self.config_input_frame.setEnabled(False)
            self.data_tx_button.setEnabled(False)
            self.file_tx_button.setEnabled(False)
            self.config_mode = False

            # Resit client attribute
            self.client = None
            
    def update_data(self, data):
        print(f"Data Recieved in Main Window: {data}")
        for i, value in enumerate(data):
            self.graph_slices[i].append(value)
        
    def update_plots(self):
        
        # Limit data to the most recent max_points
        for i, data_list in enumerate(self.graph_slices):
            # print(f"Data List: {data_list}")
            if len(data_list) > self.max_points:
                self.graph_slices[i] = data_list[-self.max_points:]
            else:
                self.graph_slices[i] = data_list
        # Generate x-axis values (sample indices)
        x_axis = self.graph_slices[0]
        print(f"X Axis: {x_axis}")
        print(f"Accel X graph data: {self.graph_slices[1]}")
        # Update accelerometer curves
        self.accel_x_curve.setData(x_axis, self.graph_slices[1])
        self.accel_y_curve.setData(x_axis, self.graph_slices[2])
        self.accel_z_curve.setData(x_axis, self.graph_slices[3])
        
        # Update gyroscope curves
        self.gyro_x_curve.setData(x_axis, self.graph_slices[4])
        self.gyro_y_curve.setData(x_axis, self.graph_slices[5])
        self.gyro_z_curve.setData(x_axis, self.graph_slices[6])
        
