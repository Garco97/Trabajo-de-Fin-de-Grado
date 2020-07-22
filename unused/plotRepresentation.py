from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure
from PyQt5.QtWidgets import QSizePolicy

import matplotlib as matplotlib
import matplotlib.pyplot as plt
import datetime
class PlotCanvas(FigureCanvas):
	def __init__(self, parent=None):
		fig = plt.figure()
		self.axes = fig.add_subplot(111)
		self.axes.set_xlabel("Time (mm:ss)")
		self.axes.set_ylabel("Chunks values")
		self.axes.set_title('Wavefront')
		FigureCanvas.__init__(self, fig)
		self.setParent(parent)
		FigureCanvas.setSizePolicy(self,
				QSizePolicy.Expanding,
				QSizePolicy.Expanding)
		FigureCanvas.updateGeometry(self)

	def plot(self, data, ticks, chunk):		
		formatter = matplotlib.ticker.FuncFormatter(timeTicks)
		self.axes.xaxis.set_major_formatter(formatter)
		aux = [min(data),max(data)]
		self.axes.plot([ticks]*2,aux,'r-')	
		self.draw()

	def clean(self):
		self.axes.clear()
		self.draw()
	
def timeTicks(self,x):
	d = datetime.timedelta(seconds=x)
	mins = int(d.seconds / 60)
	secs = d.seconds - (mins * 60)
	ms = d.microseconds * 1000
	ms_width = 2
	ms = f'{ms:05}'[0:ms_width]
	return f'{mins:02}:{secs:02}.{ms}'

