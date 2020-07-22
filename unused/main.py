from PyQt5 import QtCore, QtGui, QtWidgets
import sys

sys.path.insert(0, 'ui')
from wavefront_ui import Wavefront
global ex
if __name__ == "__main__":
	app = QtWidgets.QApplication(sys.argv)
	ex = Wavefront()
	ex.show()
	sys.exit(app.exec_())
