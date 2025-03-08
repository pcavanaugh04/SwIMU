from main_window import MainWindow
from PyQt5 import QtWidgets
import sys


if __name__ == '__main__':
    app = QtWidgets.QApplication(sys.argv)
    window = MainWindow()
    # window.resize(800, 600)
    window.setWindowTitle("BLE Accelerometer and Gyro Visualizer")
    window.show()
    sys.exit(app.exec_())