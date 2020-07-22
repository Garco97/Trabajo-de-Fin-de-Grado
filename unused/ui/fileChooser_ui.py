import sys
from PyQt5.QtWidgets import QApplication, QWidget, QInputDialog, QLineEdit, QFileDialog,QDialog
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import QFileInfo
 
class FileChooser(QDialog):
 
	def __init__(self):
		super().__init__()
		self.title = 'Choose a .wav file'
		self.left = 10
		self.top = 10
		self.width = 640
		self.height = 480
		self.file_size = None
		self.file_name = self.init_ui()
	 
	def init_ui(self):
		self.setWindowTitle(self.title)
		self.setGeometry(self.left, self.top, self.width, self.height)		 
		file_name = self.open_file_name_dialog()	 
		info = QFileInfo(file_name)
		self.file_size = info.size()
		self.show()
		return file_name
	 
	def open_file_name_dialog(self):
		options = QFileDialog.Options()
		options |= QFileDialog.DontUseNativeDialog
		file_name, _ = QFileDialog.getOpenFileName(self,self.title, "","WAV Files (*.wav)", options=options)
		if file_name:
			return file_name
	def save_file_name_dialog(self):
		options = QFileDialog.Options()
		options |= QFileDialog.DontUseNativeDialog
		file_name, _ = QFileDialog.getSaveFileName(self,self.title, "","WAV Files (*.wav)", options=options)
		if file_name:
			return file_name

class SaveChooser(QDialog):
 
	def __init__(self):
		super().__init__()
		self.title = 'Choose a .wav file'
		self.left = 10
		self.top = 10
		self.width = 640
		self.height = 480
		self.file_size = None
		self.file_name = self.init_ui()
	 
	def init_ui(self):
		self.setWindowTitle(self.title)
		self.setGeometry(self.left, self.top, self.width, self.height)		 
		file_name = self.save_file_name_dialog()	 
		info = QFileInfo(file_name)
		self.file_size = info.size()
		self.show()
		return file_name
	def save_file_name_dialog(self):
		options = QFileDialog.Options()
		options |= QFileDialog.DontUseNativeDialog
		file_name, _ = QFileDialog.getSaveFileName(self,self.title, "","WAV Files (*.wav)", options=options)
		if file_name:
			return file_name