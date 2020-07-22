import sys
import threading
import variables
import matplotlib.pyplot as plt
import wave
import contextlib

from PyQt5 import *
from PyQt5 import QtWidgets	
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
from audio import *
from fileChooser_ui import *
from plotRepresentation import *
from analyzer import *
from configuration import *
from configuration_ui import *

is_playing = False
is_recording = False
is_paused_recording = False
is_paused_playing = False
stop = False

class Wavefront(QMainWindow):
	def __init__(self,parent=None):
		super(Wavefront, self).__init__(parent)
		self.message = ""
		self.file_name  = ""
		self.my_thread = None
		self.configuration = Configuration()
		self.setup_ui()
	def setup_ui(self):
		self.setStyleSheet("QMainWindow {background: 'white';}")
		self.setObjectName("WaveFront")
		self.resize(800, 600)
		self.central_widget = QtWidgets.QWidget(self)
		self.central_widget.setObjectName("centralwidget")
		self.menubar_creation()
		self.play_pause_lbl = QtWidgets.QLabel(self.central_widget)
		self.play_pause_button = QtWidgets.QPushButton(self.central_widget)
		self.label_creation(self.play_pause_lbl, "Play", [673,25])
		self.button_creation(self.play_pause_button,[640, 25, 30, 25], "icons/play.png")
		self.play_pause_button.clicked.connect(self.play_pause_button_clicked)
		self.play_pause_button.setEnabled(False)
		self.record_lbl = QtWidgets.QLabel(self.central_widget)
		self.record_button = QtWidgets.QPushButton(self.central_widget)
		self.label_creation(self.record_lbl, "Record", [673,55])
		self.button_creation(self.record_button,[640, 55, 30, 25], "icons/record.png")
		self.record_button.clicked.connect(self.record_button_clicked)
		self.restart_lbl = QtWidgets.QLabel(self.central_widget)
		self.restart_button = QtWidgets.QPushButton(self.central_widget)
		self.label_creation(self.restart_lbl, "Restart", [673,87])
		self.button_creation(self.restart_button, [640, 87, 30, 25], "icons/restart.png")
		self.restart_button.clicked.connect(self.restart_button_clicked)
		self.plot = PlotCanvas(self.central_widget)
		self.plot.move(0,20)
		#self.plot.resize(200,200) Para un futuro, cuando mueva todo
		self.file_info_TB = QPlainTextEdit(self.central_widget)
		self.file_info_TB.move(0,500)
		self.file_info_TB.resize(800,100)
		self.file_info_TB.setObjectName("file_info")
		self.vertical_log_TB = QPlainTextEdit(self.central_widget)
		self.vertical_log_TB.move(580,150)
		self.vertical_log_TB.resize(220,300)
		self.vertical_log_TB.setObjectName("vertical_log")

		self.setCentralWidget(self.central_widget)
		quit = QAction("Quit", self)
		quit.triggered.connect(self.closeEvent)
		self.retranslate_ui()
		QtCore.QMetaObject.connectSlotsByName(self)

	def menubar_creation(self):
		self.menubar = self.menuBar()
		self.preferences_menu_creation()
		self.file_menu_creation()
	
	def label_creation(self, label, text, geometry):
		label.setText(text)
		label.move(geometry[0], geometry[1])

	def button_creation(self, button, geometry, icon_path):
		button.setGeometry(QtCore.QRect(geometry[0], geometry[1], geometry[2], geometry[3]))
		icon = QtGui.QIcon()
		icon.addPixmap(QtGui.QPixmap(icon_path), QtGui.QIcon.Normal, QtGui.QIcon.Off)
		button.setIcon(icon)
		
	def preferences_menu_creation(self):
		preferences_menu = self.menubar.addMenu("Preferences")
		configuration_menu_action = QtWidgets.QAction('Configuration', self)
		configuration_menu_action.triggered.connect(self.open)
		preferences_menu.addAction(configuration_menu_action)
		recording_device_menu = QtWidgets.QMenu('Recording Device', self)
		preferences_menu.addMenu(recording_device_menu)
		self.recording_device_menu_creation(recording_device_menu)
		listening_device_menu = QtWidgets.QMenu('Listening Device', self)
		preferences_menu.addMenu(listening_device_menu)
		self.listening_device_menu_creation(listening_device_menu)

	def listening_device_menu_creation(self,menu):
		menu.clear()
		p = pyaudio.PyAudio()
		info = p.get_host_api_info_by_index(0)
		num_devices = info.get('deviceCount')
		devices = set([])
		self.output_devices = {}
		for i in range(0, num_devices):
			if p.get_device_info_by_host_api_device_index(0, i).get('maxOutputChannels') > 0:
				info = p.get_device_info_by_host_api_device_index(0, i)
				action = QtWidgets.QAction(info.get('name'), self)
				action.triggered.connect(lambda checked, device=info.get('name'): self.change_output_device(device,menu))
				menu.addAction(action)
				devices.add(action)
				self.output_devices[info.get('name')] = info.get('index')
				if info.get('name') == self.configuration.output_device:
					self.set_non_checkable(devices)
					action.setCheckable(True)
					action.setChecked(True)

	def recording_device_menu_creation(self,menu):
		menu.clear()
		p = pyaudio.PyAudio()
		info = p.get_host_api_info_by_index(0)
		num_devices = info.get('deviceCount')
		devices = set([])
		self.input_devices = {}
		for i in range(0, num_devices):
			if p.get_device_info_by_host_api_device_index(0, i).get('maxInputChannels') > 0:
				info = p.get_device_info_by_host_api_device_index(0, i)
				action = QtWidgets.QAction(info.get('name'), self)
				action.triggered.connect(lambda checked, device=info.get('name'): self.change_input_device(device,menu))
				menu.addAction(action)
				devices.add(action)
				self.input_devices[info.get('name')] = info.get('index')

				if info.get('name') == self.configuration.input_device:
					self.set_non_checkable(devices)
					action.setCheckable(True)
					action.setChecked(True)
	
	def change_input_device(self, device, menu):
		self.configuration.input_device = device
		self.configuration.input_device_index = self.input_devices[device]
		self.configuration.write_cache()
		self.recording_device_menu_creation(menu)

	def change_output_device(self, device, menu):
		self.configuration.output_device = device
		self.configuration.output_device_index = self.output_devices[device]
		self.configuration.write_cache()
		self.listening_device_menu_creation(menu)

	def set_non_checkable(self,devices):
		for device in devices:
			device.setCheckable(False)

	def file_menu_creation(self):
		file_menu = self.menubar.addMenu("File")
		open_menu_action = QtWidgets.QAction('Open file ...', self)
		save_menu_action = QtWidgets.QAction('Save file ...', self)
		open_menu_action.triggered.connect(self.open_file)
		save_menu_action.triggered.connect(self.save_file)
		file_menu.addAction(open_menu_action)
		file_menu.addAction(save_menu_action)

	def retranslate_ui(self,):
		_translate = QtCore.QCoreApplication.translate
		self.setWindowTitle(_translate("WaveFront", "WaveFront"))

	def open(self):
		dialog = Configuration_UI(self)
		dialog.show()

	def open_file(self):
		ex = FileChooser()
		self.file_name = ex.file_name
		name = self.file_name.split("/")[-1]
		file_format = name.split(".")[1]
		with contextlib.closing(wave.open(self.file_name,'r')) as f:
			frames = f.getnframes()
			rate = f.getframerate()
			duration = frames / float(rate)
		self.file_info_TB.insertPlainText("Nombre del fichero " + name + 
			" Tamaño " + str(round(ex.file_size/(1024),3)) + "KB " + 
			" Formato " + file_format + " Duracion " + str(round(duration,2)) + " segundos \n")
		self.play_pause_button.setEnabled(True)
		self.record_button.setEnabled(False)
			
	def play_pause_button_clicked(self):
		global is_playing
		global is_recording
		global is_paused_recording
		global is_paused_playing
		self.configuration = Configuration()

		if not is_recording:
			if not is_playing:
				is_playing = True	
				self.set_button_icon(self.play_pause_button, "icons/pause.png")
				self.plot.clean()

				if self.configuration.channel != 0 and self.configuration.channel != 1 and self.configuration.channel != 2:
					text, okPressed = QInputDialog.getInt(self, "Channel","Choose the channel ((default)0 or 1) or channels(2):", QLineEdit.Normal, 0)
				else:
					text = self.configuration.channel
					okPressed = True
				if text != 0 and text != 1 and text != 2:
					text = 0 
				while not okPressed :
					text, okPressed = QInputDialog.getInt(self, "Channel","Choose the channel ((default)0 or 1) or channels(2):", QLineEdit.Normal, 0)
					self.configuration.write_configuration()
				self.play_pause_lbl.setText("Pause")

				self.configuration.channel = text
				is_playing = True
				play = PlayAudio(self.file_name,self)
				play.finish_play_thread.connect(self.finish_play_thread)
				play.analyzer = Analyzer()
				play.analyzer.bigger.connect(self.sayAnalyzer)
				self.my_thread = threading.Thread(target = play.play_audio)
				self.my_thread.daemon = True
				self.my_thread.start()	
	
			else:
				if is_paused_playing:
					self.play_pause_lbl.setText("Pause")
					is_paused_playing = False
					self.set_button_icon(self.play_pause_button, "icons/pause.png")
	
				else:
					self.play_pause_lbl.setText("Play")
					is_paused_playing = True
					self.set_button_icon(self.play_pause_button, "icons/play.png")

	def record_button_clicked(self):
		global is_playing
		global is_recording
		global is_paused_recording
		self.configuration = Configuration()
		if not is_playing:
			if not is_recording:
				self.plot.clean()
				self.file_info_TB.clear()
				self.play_pause_button.setEnabled(False)
				self.set_button_icon(self.record_button, "icons/recordPause.png")
				self.record_lbl.setText("Pause record")
				is_recording = True
				self.file_info_TB.insertPlainText("Recording\n")
				play = PlayAudio('',self)
				play.finish_record_thread.connect(lambda data: self.finish_record_thread(data))
				play.analyzer = Analyzer()
				play.analyzer.bigger.connect(self.sayAnalyzer)
				self.my_thread = threading.Thread(target = play.record_audio)
				self.my_thread.daemon = True
				self.my_thread.start()

			else:
				if is_paused_recording:
					self.record_lbl.setText("Pause record")
					self.file_info_TB.insertPlainText("Recording resumed\n")
					is_paused_recording = False
					self.set_button_icon(self.record_button, "icons/recordPause.png")
				else:
					self.record_lbl.setText("Record")
					self.file_info_TB.insertPlainText("Recording paused\n")
					is_paused_recording = True
					self.set_button_icon(self.record_button, "icons/record.png")
					
	def save_file(self):
		global is_paused_recording
		global is_recording
		self.configuration = Configuration()
		if is_paused_recording:
			is_recording = False
			is_paused_recording = False
			self.file_info_TB.insertPlainText("Recording stopped\n")

	def restart_button_clicked(self):
		global is_paused_playing
		global is_paused_recording
		global is_playing
		global is_recording
		global stop
		self.configuration = Configuration()
		if is_paused_playing or is_paused_recording or not is_recording or not is_playing:
			if is_paused_recording and self.configuration.overwrite:
				reply = QMessageBox.question(self, '', '¿Estás seguro de que deseas eliminar la grabación?')
				if reply == QMessageBox.No:
					return
			stop = True
			is_paused_playing = False
			is_paused_recording = False
			is_playing = False
			is_recording = False
			self.my_thread.join()
			self.vertical_log_TB.clear()
			self.plot.clean()
			self.record_lbl.setText("Record")
			self.play_pause_lbl.setText("Play")
			self.play_pause_button.setEnabled(False)
			self.record_button.setEnabled(True)
			stop = False

	def sayAnalyzer(self):
		self.vertical_log_TB.insertPlainText("Chunk " + str(variables.chunk_number) + " is bigger than " + 
			str(variables.threshold) + "\n")

	def finish_play_thread(self):
		self.my_thread.join()
		self.play_pause_lbl.setText("Play")
		self.set_button_icon(self.play_pause_button, "icons/play.png")
	
	def finish_record_thread(self, data):
		if data != []:
			ex = SaveChooser()
			self.file_name = ex.file_name
			wave_file = wave.open(self.file_name + '.wav', 'wb')
			wave_file.setnchannels(data[0])
			wave_file.setsampwidth(data[1])
			wave_file.setframerate(data[2])
			wave_file.writeframes(data[3])
			wave_file.close()
			self.file_info_TB.insertPlainText("Fichero guardado en: " + self.file_name)
		self.my_thread.join()
		self.set_button_icon(self.record_button, "icons/record.png")

	def set_button_icon(self, button, image):
		icon = QtGui.QIcon()
		icon.addPixmap(QtGui.QPixmap(image), QtGui.QIcon.Normal, QtGui.QIcon.Off)
		button.setIcon(icon)

	def closeEvent(self, event):
		self.configuration = Configuration()
		if self.configuration.delete_cache_on_close: 
			if os.path.exists(".cache.json"):
				os.remove(".cache.json")