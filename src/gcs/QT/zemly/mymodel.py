# This Python file uses the following encoding: utf-8

from PySide6 import QtGui
from itertools import chain
import numpy
from stl import mesh
import pyqtgraph as pg
import pyqtgraph.opengl as Opengl


class mymodel(Opengl.GLViewWidget):

    def __init__(self):
        super(mymodel, self).__init__()
        self.axis = Opengl.GLAxisItem()
        self.addItem(self.axis)
        self.axis.show()
        self.grid = Opengl.GLGridItem()
        self.addItem(self.grid)
        self.grid.setSize(20, 20)
        self.grid.show()

        self.mesh = Opengl.GLMeshItem()
        self.addItem(self.mesh)

        stl = mesh.Mesh.from_file("C:/Users/Anton/Documents/cansat-KNPN-S/src/gcs/QT/zemly/glider_skel_for_anton.stl")
        points = stl.points
        points = numpy.array(list(chain(*points)))
        nd_points = numpy.ndarray(shape=(len(points) // 3, 3, ))
        for i in range(0, len(points) // 3):
            nd_points[i] = points[i*3:(i+1)*3]

        faces = numpy.array([(i, i+1, i+2,) for i in range(0, len(nd_points), 3)])
        colors = numpy.array([(0.5, 0.5, 0.5, 1,) for i in range(0, len(nd_points), 3)])

        self.mesh.setMeshData(vertexes=nd_points,
                              faces=faces,
                              faceColors=colors,
                              edgeColor=(0, 0, 0, 1),
                              drawFaces=True,
                              drawEdges=False,
                              shader="edgeHilight",
                              smooth=False,)


    def rotate(self, data):
        quat = QtGui.QQuaternion(*data)
        self.meah.resetTransform()
        axis, angle = quat.getAxisandAngle()
        self.mesh.rotate(angle, axis[0], axis[1], axis[2])

