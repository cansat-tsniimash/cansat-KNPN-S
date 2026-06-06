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
from PySide6.QtGui import QPen, Qt



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


    udp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    MA = Signal(list)
    DA0 = Signal(list)
    DA1 = Signal(list)
    DA2 = Signal(list)
    mutex = QMutex()

    def run(self):

        data = 0
        self.udp.connect(("192.168.0.203", 20003))
        self.udp.sendto("h".encode('utf-8'), ("192.168.0.203", 20003))
        num = 0
        while True:
            self.udp.sendto("h".encode('utf-8'), ("192.168.0.203", 20003))

            data = self.udp.recv(55)
            flug_cond = struct.unpack("B", data[:1])[0]
            if flug_cond == 0xAA:
                data = struct.unpack("<B2HI9hIh2H3fBHBH", data[:55])
                self.MA.emit(data)

            elif flug_cond == 0xBB:
                data = struct.unpack("<BHI9hBH", data)
                print ("flug", unpack_data[0])
                print ("Number", unpack_data[1])
                print ("Time_ms", unpack_data[2])

                for i in range(3):
                   unpack_data[3 + i] = unpack_data[3 + i]*70/1000
                print ("Gyroscope x", unpack_data[3])
                print ("Gyroscope y", unpack_data[4])
                print ("Gyroscope z", unpack_data[5])
                for i in range(3):
                   unpack_data[6 + i] = unpack_data[6 + i]*488/1000/1000
                print ("Accelerometer x", unpack_data[6])
                print ("Accelerometer y", unpack_data[7])
                print ("Accelerometer z", unpack_data[8])
                for i in range(3):
                   unpack_data[9 + i] = unpack_data[9 + i]/1711
                print ("Magnetometer x", unpack_data[9])
                print ("Magnetometer y", unpack_data[10])
                print ("Magnetometer z", unpack_data[11])
         #   print ("Gyroscope x", unpack_data[4])
         #   print ("Gyroscope y", unpack_data[5])
         #   print ("Gyroscope z", unpack_data[6])
         #   print ("Accelerometer x", unpack_data[7])
         #   print ("Accelerometer y", unpack_data[8])
         #   print ("Accelerometer z", unpack_data[9])
         #   print ("Magnetometer x", unpack_data[10])
         #   print ("Magnetometer y", unpack_data[11])
         #   print ("Magnetometer z", unpack_data[12])

                print ("Status", unpack_data[12])
                print ("crc", unpack_data[13])
            elif flug_cond == 0xCC:
                data = struct.unpack("<BHI3fB2H", data)
                print ("flug", unpack_data[0])
                print ("Number", unpack_data[1])
                print ("Time_ms", unpack_data[2])

                print ("GPS_lat", unpack_data[3])
                print ("GPS_lon", unpack_data[4])
                print ("GPS_height", unpack_data[5])
                print ("GPS_fix", unpack_data[6])

                print ("Photo", unpack_data[7])
                print ("crc", unpack_data[8])

            elif flug_cond == 0xDD:
                data = struct.unpack("<BH3I2h3HBH", data)
                print ("flug", unpack_data[0])
                print ("Number", unpack_data[1])
                print ("Time_ms", unpack_data[2])

                print ("Bme_press_1", unpack_data[3])
                print ("Bme_press_2", unpack_data[4])
                for i in range(2):
                    unpack_data[5 + i] = unpack_data[5 + i]/100
                print ("Bme_temp_1", unpack_data[5])
                print ("Bme_temp_2", unpack_data[6])
                for i in range(2):
                    unpack_data[7 + i] = unpack_data[7 + i]/10
                print ("Bme_humidity_1", unpack_data[7])
                print ("Bme_humidity_2", unpack_data[8])
                print ("Bme_height", unpack_data[9])

                print ("Pito_speed", unpack_data[10])
                print ("crc", unpack_data[11])
            else:
                print ("!")
                data = data[1:]





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
        axis_y.setLabel("alt (метры)")
        self.ui.graph.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph_list = [self.ui.graph.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("temp (Градусы Цельсия)")
        self.ui.graph2.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph2_list = [self.ui.graph2.plot(), self.ui.graph2.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("mag (Гауссы)")
        self.ui.graph3.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph3_list = [self.ui.graph3.plot(), self.ui.graph3.plot(), self.ui.graph3.plot()]


        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("ACC (G)")
        self.ui.graph4.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph4_list = [self.ui.graph4.plot(), self.ui.graph4.plot(), self.ui.graph4.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("GYR (Градусы в секунду)")
        self.ui.graph5.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph5_list = [self.ui.graph5.plot(), self.ui.graph5.plot(), self.ui.graph5.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("Pressure (Паскали)")
        self.ui.graph6.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graph6_list = [self.ui.graph6.plot()]



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

        self.plot_data_upload(self.graph_list[0], data[3], data[16])

        self.plot_data_upload(self.graph2_list[0], data[3], data[14])

        self.graph3_list[0].setPen(pg.mkPen(color='b', width=2))  # синий
        self.graph3_list[1].setPen(pg.mkPen(color='g', width=2))  # зеленый
        self.graph3_list[2].setPen(pg.mkPen(color='r', width=2))  # красный
        self.plot_data_upload(self.graph3_list[0], data[3], data[10])
        self.plot_data_upload(self.graph3_list[1], data[3], data[11])
        self.plot_data_upload(self.graph3_list[2], data[3], data[12])


        self.graph5_list[0].setPen(pg.mkPen(color='y', width=2))  # жёлтый
        self.graph5_list[1].setPen(pg.mkPen(color='g', width=2))  # зеленый
        self.graph5_list[2].setPen(pg.mkPen(color='r', width=2))  # красный
        self.plot_data_upload(self.graph5_list[0], data[3], data[4])
        self.plot_data_upload(self.graph5_list[1], data[3], data[5])
        self.plot_data_upload(self.graph5_list[2], data[3], data[6])

        self.graph4_list[0].setPen(pg.mkPen(color='w', width=2))  # оливковый
        self.graph4_list[1].setPen(pg.mkPen(color='g', width=2))  # зеленый
        self.graph4_list[2].setPen(pg.mkPen(color='r', width=2))  # красный
        self.plot_data_upload(self.graph4_list[0], data[3], data[7])
        self.plot_data_upload(self.graph4_list[1], data[3], data[8])
        self.plot_data_upload(self.graph4_list[2], data[3], data[9])

if __name__ == "__main__":
    app = QApplication([])
    widget = zemla()
    #widget.show()
    sys.exit(app.exec_())




