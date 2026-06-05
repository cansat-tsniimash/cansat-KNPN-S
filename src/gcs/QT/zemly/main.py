# This Python file uses the following encoding: utf-8
import sys
import os
import time
import pyqtgraph as pg
import numpy
import math
import socket
import struct
from mymodel import mymodel

from PySide6.QtWidgets import QApplication, QMainWindow, QTableWidgetItem
from PySide6.QtCore import QFile, QObject, Signal, QThread, QMutex
from PySide6.QtUiTools import QUiLoader



class DataManager(QObject):
    def crc16(data : bytearray, offset=0, length=-1):
        if length < 0:
            length = len(data)

        if data is None or offset < 0 or offset > len(data)- 1 and offset+length > len(data):
            return 0

        crc = 0xFFFF
        for i in range(0, length):
            crc ^= data[offset + i] << 8
            for j in range(0, 8):
                if (crc & 0x8000) > 0:
                    crc = (crc << 1) ^ 0x1021
                else:
                    crc = crc << 1
            crc = crc & 0xFFFF
        return crc & 0xFFFF
    #self.udp_socket.setblocking(False)
   # self.udp_socket.settimeout(0.1)

    buf = bytes()
    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    MA = Signal(list)
    DA0 = Signal(list)
    DA1 = Signal(list)
    DA2 = Signal(list)
    mutex = QMutex()

    def run(self):
        buffers = 0
        data = None
        self.udp.connect(("192.168.0.203", 20003))
        self.udp.sendto("h".encode('utf-8'), ("192.168.0.203", 20003))
        num = 0
        alt = 0
        while True:
            self.udp.sendto("h".encode('utf-8'), ("192.168.0.203", 20003))
            print(data)
            data = self.udp.recv(58)
            data = struct.unpack("<B2HI9hIh2H3fBHBH", data)
            self.MA.emit(data)
            time.sleep(1)


    def start(self):
       host = '192.168.0.203'
       port = 20003
       self.addr = (host,port)


       #self.udp_socket.setblocking(False)
      # self.udp_socket.settimeout(0.1)


       print("Yes")

       self.mutex.lock()
       self.close = True
       close = self.close
       self.mutex.unlock()

    def reconnect(self):
      self.udp_socket.sendto(b"h", self.addr)










class UiLoader(QUiLoader):
    def createWidget(self, className, parent=None, name =""):
        if className == "PlotWidget":
            return pg.PlotWidget(parent=parent)
        if className == "mymodel":
            return mymodel()
        return super().createWidget(className, parent, name)

class zemla(QMainWindow):
    def __init__(self):
        super(zemla, self).__init__()
        self.load_ui()
        self.ui.show()
        self.ui.centralwidget.layout().addWidget(mymodel(), 1, 1)
        self.data_thread = QThread(self)
        self.data_manager = DataManager()
        self.data_manager.moveToThread(self.data_thread)
        self.data_thread.started.connect(self.data_manager.run)
        self.data_thread.start()
        self.data_manager.MA.connect(self.setdatapachet)
        self.ui.model.show()

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("alt")
        self.ui.graph.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph_list = [self.ui.graph.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("temp")
        self.ui.graph.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph_list = [self.ui.graph2.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("mag")
        self.ui.graph.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph_list = [self.ui.graph3.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("ACC")
        self.ui.graph.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph_list = [self.ui.graph4.plot()]

    def plot_data_upload(self, plot, x, y):
        newX, newY = plot.getData()
        if (newX is None):
            plot.setData(numpy.array([x]) , numpy.array([y]))
            return
        x = numpy.append(newX, x)
        y = numpy.append(newY, y)
        plot.setData(x , y)


    def load_ui(self):
        loader = UiLoader()
        path = os.path.join(os.path.dirname(__file__), "form.ui")
        ui_file = QFile(path)
        ui_file.open(QFile.ReadOnly)
        self.ui = loader.load(ui_file, self)
        ui_file.close()

    def setdatapachet(self, data):
        self.ui.packet.setItem(0 , 0 , QTableWidgetItem(str(data[0])))
        self.ui.packet.setItem(1 , 0 , QTableWidgetItem(str(data[1])))
        self.plot_data_upload(self.graph_list[0], data[0], data[1])

if __name__ == "__main__":
    app = QApplication([])
    widget = zemla()
    #widget.show()
    sys.exit(app.exec_())




