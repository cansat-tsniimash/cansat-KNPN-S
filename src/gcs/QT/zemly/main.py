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


class MadgwickAHRS:
    def __init__(self, beta=0.1):
        self.beta = beta

        # q0, q1, q2, q3 == w, x, y, z
        self.quaternion = [1.0, 0.0, 0.0, 0.0]

    @staticmethod
    def inv_sqrt(x):
        return 1.0 / math.sqrt(x)

    def update_imu(self, gx, gy, gz, ax, ay, az, dt):
        q0, q1, q2, q3 = self.quaternion
        beta = self.beta

        q_dot1 = 0.5 * (-q1 * gx - q2 * gy - q3 * gz)
        q_dot2 = 0.5 * ( q0 * gx + q2 * gz - q3 * gy)
        q_dot3 = 0.5 * ( q0 * gy - q1 * gz + q3 * gx)
        q_dot4 = 0.5 * ( q0 * gz + q1 * gy - q2 * gx)

        if not (ax == 0.0 and ay == 0.0 and az == 0.0):
            recip_norm = self.inv_sqrt(ax * ax + ay * ay + az * az)
            ax *= recip_norm
            ay *= recip_norm
            az *= recip_norm

            _2q0 = 2.0 * q0
            _2q1 = 2.0 * q1
            _2q2 = 2.0 * q2
            _2q3 = 2.0 * q3
            _4q0 = 4.0 * q0
            _4q1 = 4.0 * q1
            _4q2 = 4.0 * q2
            _8q1 = 8.0 * q1
            _8q2 = 8.0 * q2

            q0q0 = q0 * q0
            q1q1 = q1 * q1
            q2q2 = q2 * q2
            q3q3 = q3 * q3

            s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay
            s1 = (_4q1 * q3q3 - _2q3 * ax + 4.0 * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az)
            s2 = (4.0 * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az)
            s3 = (4.0 * q1q1 * q3 - _2q1 * ax + 4.0 * q2q2 * q3 - _2q2 * ay)

            norm = s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3
            if norm != 0.0:
                recip_norm = self.inv_sqrt(norm)
                s0 *= recip_norm
                s1 *= recip_norm
                s2 *= recip_norm
                s3 *= recip_norm

                q_dot1 -= beta * s0
                q_dot2 -= beta * s1
                q_dot3 -= beta * s2
                q_dot4 -= beta * s3

        q0 += q_dot1 * dt
        q1 += q_dot2 * dt
        q2 += q_dot3 * dt
        q3 += q_dot4 * dt

        recip_norm = self.inv_sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3)

        self.quaternion = [q0 * recip_norm, q1 * recip_norm, q2 * recip_norm, q3 * recip_norm,]

        return self.quaternion

    def update(self, gx, gy, gz, ax, ay, az, mx, my, mz, dt):
        if mx == 0.0 and my == 0.0 and mz == 0.0:
            return self.update_imu(gx, gy, gz, ax, ay, az, dt)

        q0, q1, q2, q3 = self.quaternion
        beta = self.beta

        q_dot1 = 0.5 * (-q1 * gx - q2 * gy - q3 * gz)
        q_dot2 = 0.5 * ( q0 * gx + q2 * gz - q3 * gy)
        q_dot3 = 0.5 * ( q0 * gy - q1 * gz + q3 * gx)
        q_dot4 = 0.5 * ( q0 * gz + q1 * gy - q2 * gx)

        if not (ax == 0.0 and ay == 0.0 and az == 0.0):
            recip_norm = self.inv_sqrt(ax * ax + ay * ay + az * az)
            ax *= recip_norm
            ay *= recip_norm
            az *= recip_norm

            recip_norm = self.inv_sqrt(mx * mx + my * my + mz * mz)
            mx *= recip_norm
            my *= recip_norm
            mz *= recip_norm

            _2q0mx = 2.0 * q0 * mx
            _2q0my = 2.0 * q0 * my
            _2q0mz = 2.0 * q0 * mz
            _2q1mx = 2.0 * q1 * mx

            _2q0 = 2.0 * q0
            _2q1 = 2.0 * q1
            _2q2 = 2.0 * q2
            _2q3 = 2.0 * q3

            q0q0 = q0 * q0
            q0q1 = q0 * q1
            q0q2 = q0 * q2
            q0q3 = q0 * q3
            q1q1 = q1 * q1
            q1q2 = q1 * q2
            q1q3 = q1 * q3
            q2q2 = q2 * q2
            q2q3 = q2 * q3
            q3q3 = q3 * q3

            hx = (mx * q0q0 - _2q0my * q3 + _2q0mz * q2 + mx * q1q1 + _2q1 * my * q2 + _2q1 * mz * q3 - mx * q2q2 - mx * q3q3)

            hy = (_2q0mx * q3 + my * q0q0 - _2q0mz * q1 + _2q1mx * q2 - my * q1q1 + my * q2q2 + _2q2 * mz * q3 - my * q3q3)

            _2bx = math.sqrt(hx * hx + hy * hy)

            _2bz = (-_2q0mx * q2 + _2q0my * q1 + mz * q0q0 + _2q1mx * q3 - mz * q1q1 + _2q2 * my * q3 - mz * q2q2 + mz * q3q3)

            _4bx = 2.0 * _2bx
            _4bz = 2.0 * _2bz
            _8bx = 2.0 * _4bx
            _8bz = 2.0 * _4bz

            s0 = (-_2q2 * (2.0 * (q1q3 - q0q2) - ax) + _2q1 * (2.0 * (q0q1 + q2q3) - ay) - _4bz * q2 * (_4bx * (0.5 - q2q2 - q3q3) + _4bz * (q1q3 - q0q2) - mx) + (-_4bx * q3 + _4bz * q1) * (_4bx * (q1q2 - q0q3) + _4bz * (q0q1 + q2q3) - my) + _4bx * q2 * (_4bx * (q0q2 + q1q3) + _4bz * (0.5 - q1q1 - q2q2) - mz))

            s1 = (_2q3 * (2.0 * (q1q3 - q0q2) - ax) + _2q0 * (2.0 * (q0q1 + q2q3) - ay) - 4.0 * q1 * (2.0 * (0.5 - q1q1 - q2q2) - az) + _4bz * q3 * (_4bx * (0.5 - q2q2 - q3q3) + _4bz * (q1q3 - q0q2) - mx) + (_4bx * q2 + _4bz * q0) * (_4bx * (q1q2 - q0q3) + _4bz * (q0q1 + q2q3) - my) + (_4bx * q3 - _8bz * q1) * (_4bx * (q0q2 + q1q3) + _4bz * (0.5 - q1q1 - q2q2) - mz))

            s2 = (-_2q0 * (2.0 * (q1q3 - q0q2) - ax) + _2q3 * (2.0 * (q0q1 + q2q3) - ay) - 4.0 * q2 * (2.0 * (0.5 - q1q1 - q2q2) - az) + (-_8bx * q2 - _4bz * q0) * (_4bx * (0.5 - q2q2 - q3q3) + _4bz * (q1q3 - q0q2) - mx) + (_4bx * q1 + _4bz * q3) * (_4bx * (q1q2 - q0q3) + _4bz * (q0q1 + q2q3) - my) + (_4bx * q0 - _8bz * q2) * (_4bx * (q0q2 + q1q3) + _4bz * (0.5 - q1q1 - q2q2) - mz))

            s3 = (_2q1 * (2.0 * (q1q3 - q0q2) - ax) + _2q2 * (2.0 * (q0q1 + q2q3) - ay) + (-_8bx * q3 + _4bz * q1) * (_4bx * (0.5 - q2q2 - q3q3) + _4bz * (q1q3 - q0q2) - mx) + (-_4bx * q0 + _4bz * q2) * (_4bx * (q1q2 - q0q3) + _4bz * (q0q1 + q2q3) - my) + (_4bx * q1) * (_4bx * (q0q2 + q1q3) + _4bz * (0.5 - q1q1 - q2q2) - mz))

            norm = s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3
            if norm != 0.0:
                recip_norm = self.inv_sqrt(norm)
                s0 *= recip_norm
                s1 *= recip_norm
                s2 *= recip_norm
                s3 *= recip_norm

                q_dot1 -= beta * s0
                q_dot2 -= beta * s1
                q_dot3 -= beta * s2
                q_dot4 -= beta * s3

        q0 += q_dot1 * dt
        q1 += q_dot2 * dt
        q2 += q_dot3 * dt
        q3 += q_dot4 * dt

        recip_norm = self.inv_sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3)

        self.quaternion = [q0 * recip_norm, q1 * recip_norm, q2 * recip_norm, q3 * recip_norm,]

        return self.quaternion

    def get_quat(self):
        return self.quaternion

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
                quaternion = madgwick.update(gx, gy, gz, data[7], data[8], az, mx, my, mz, dt)


            elif flug_cond == 0xBB:
                datada1 = struct.unpack("<BHI9hBH", datada1)

            elif flug_cond == 0xCC:
                datada2 = struct.unpack("<BHI3fB2H", datada2)


            elif flug_cond == 0xDD:
                datada3 = struct.unpack("<BH3I2h3HBH", datada3)

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
        axis_x.setLabel("time (секунды)")
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

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("alt (метры)")
        self.ui.graphda2.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graphda2_list = [self.ui.graphda2.plot()]

        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("GYR (Градусы в секунду)")
        self.ui.graphda1.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graphda1_list = [self.ui.graphda1.plot(), self.ui.graphda1.plot(), self.ui.graphda1.plot()]


        self.ui.show()
        self.data_manager.MA.connect(self.setdatapachet)
        axis_x = pg.AxisItem("bottom")
        axis_x.setLabel("time (секунды)")
        axis_y = pg.AxisItem("left")
        axis_y.setLabel("ACC (G)")
        self.ui.graphda3.setAxisItems({"bottom":axis_x, "left":axis_y})
        self.graphda3_list = [self.ui.graphda3.plot(), self.ui.graphda3.plot(), self.ui.graphda3.plot()]


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

        self.graph4_list[0].setPen(pg.mkPen(color='w', width=2))  # белый
        self.graph4_list[1].setPen(pg.mkPen(color='g', width=2))  # зеленый
        self.graph4_list[2].setPen(pg.mkPen(color='r', width=2))  # красный
        self.plot_data_upload(self.graph4_list[0], data[3], data[7])
        self.plot_data_upload(self.graph4_list[1], data[3], data[8])
        self.plot_data_upload(self.graph4_list[2], data[3], data[9])

        self.plot_data_upload(self.graphda2_list[2], datada1[2], datada3[9])

        self.graphda1_list[0].setPen(pg.mkPen(color='w', width=2))  # белый
        self.graphda1_list[1].setPen(pg.mkPen(color='g', width=2))  # зеленый
        self.graphda1_list[2].setPen(pg.mkPen(color='r', width=2))  # красный
        self.plot_data_upload(self.graphda1_list[0], datada1[2], datada1[3])
        self.plot_data_upload(self.graphda1_list[1], datada1[2], datada1[4])
        self.plot_data_upload(self.graphda1_list[2], datada1[2], datada1[5])

        self.graphda3_list[0].setPen(pg.mkPen(color='y', width=2))  # белый
        self.graphda3_list[1].setPen(pg.mkPen(color='g', width=2))  # зеленый
        self.graphda3_list[2].setPen(pg.mkPen(color='r', width=2))  # красный
        self.plot_data_upload(self.graphda3_list[0], datada1[2], datada1[6])
        self.plot_data_upload(self.graphda3_list[1], datada1[2], datada1[7])
        self.plot_data_upload(self.graphda3_list[2], datada1[2], datada1[8])





if __name__ == "__main__":
    app = QApplication([])
    widget = zemla()
    #widget.show()
    sys.exit(app.exec_())




