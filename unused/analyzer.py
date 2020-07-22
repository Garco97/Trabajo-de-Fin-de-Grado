import numpy as np
import variables
from PyQt5 import *
from PyQt5.QtCore import *
class Analyzer(QObject):
	bigger = QtCore.pyqtSignal()
	def __init__(self):
		QtCore.QThread.__init__(self)
		self.window_size = 8
		self.threshold = None
		self.canvas = None
		self.chunk_queue = []
		self.min_true = 4	
		self.current_true = 0
		self.index = 0

	
	def run(self):
		self.current_true = 0

		for i in range(self.index,self.index + self.window_size):
			if self.min_true - self.current_true > self.index + self.window_size - i:
				break
			if self.current_true == self.min_true:
				break 
			if self.chunk_queue[i] > self.threshold:
				self.current_true +=1

		if self.current_true >= self.min_true:

			variables.threshold = self.threshold
			variables.chunk_number = self.index

			self.bigger.emit()




