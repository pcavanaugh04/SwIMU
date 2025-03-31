import matplotlib.pyplot as plt
from PyQt5.QtWidgets import QApplication, QFileDialog
import pandas as pd
import numpy as np
import os
import sys

def select_file():
    app = QApplication(sys.argv)
    file_path, _ = QFileDialog.getOpenFileName(None, "Select a file")
    return file_path

if __name__ == "__main__":
    selected_file = select_file()
    file_name = os.path.split(selected_file)[1]

    if selected_file:
        print(f"You selected: {selected_file}")
    else:
        print("No file selected")
    # import contents of file to a pandas dataframe and plot it
    headers = ['elapsed_time','Ax', 'Ay', 'Az', 'Gx', 'Gy', 'Gz']
    data = pd.read_csv(selected_file, header=None, names=headers, dtype=float)
    # create a new column with the magnitude of the acceleration
    data['Amag'] = np.sqrt(data['Ax']**2 + data['Ay']**2 + data['Az']**2)
    # plot the magnitude of acceleration
    plt.figure(figsize=(8,10))
    # plt.plot(data['elapsed_time'], data['Amag'])
    plt.plot(data['elapsed_time'], data['Ax'], label='Ax')
    plt.plot(data['elapsed_time'], data['Ay'], label='Ay')
    plt.plot(data['elapsed_time'], data['Az'], label='Az')
    plt.legend()
    plt.xlabel('Time (s)')
    plt.ylabel('Acceleration (m/s^2)')
    plt.title(f'Acceleration for file {file_name}')
    plt.show()

