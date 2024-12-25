# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

# Bluetooth LE scanner
# Prints the name and address of every nearby Bluetooth LE device

import asyncio
import nest_asyncio
from bleak import BleakScanner, BleakClient

IMU_DATA_UUID = "550e8401-e29b-41d4-a716-446655440002"


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
    
    async with BleakClient(address, timeout=20) as client:
        print("Device Connected!")

        print(client)
        services = client.services
        for service in services:
            print(f"Serice: {service}")
            for char in service.characteristics:
                print(f" Characteristic: {char}")
        
        while True:
            imu_sensor_values = await client.read_gatt_char(IMU_DATA_UUID)
            print(imu_sensor_values)

        
if __name__ == "__main__":
    asyncio.run(main())