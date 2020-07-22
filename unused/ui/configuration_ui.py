from PyQt5 import *
from PyQt5.QtWidgets import *
from PyQt5.QtGui import *
import pyaudio


class Configuration_UI(QMainWindow):
	def __init__(self, parent=None):
		super(Configuration_UI, self).__init__(parent)
		self.parent = parent
		self.setup_ui()
	def setup_ui(self):
		self.setStyleSheet("QMainWindow {background: 'white';}")
		self.setObjectName("Configuration")
		self.resize(720, 540)
		self.central_widget = QtWidgets.QWidget(self)
		self.central_widget.setObjectName("centralwidget")
		self.setCentralWidget(self.central_widget)
		channel_lbl = QtWidgets.QLabel(self)
		channel_lbl.setText("Channel:")
		channel_lbl.move(0,0)
		channel_cb = QComboBox(self)
		channel_cb.addItem("0")
		channel_cb.addItem("1")
		channel_cb.addItem("2")
		channel_cb.activated[int].connect(self.save_channel)
		channel_cb.setCurrentIndex(self.parent.configuration.channel)
		channel_cb.move(59,	0)

		input_device = QtWidgets.QLabel(self)
		input_device.setText("Input device:")
		input_device.move(0,30)
		input_device_cb = QComboBox(self)
		input_device_cb.activated[str].connect(self.save_input_device)

		p = pyaudio.PyAudio()
		info = p.get_host_api_info_by_index(0)
		num_devices = info.get('deviceCount')
		self.input_devices = {}
		for i in range(0, num_devices):
				if (p.get_device_info_by_host_api_device_index(0, i).get('maxInputChannels')) > 0:
					info  = p.get_device_info_by_host_api_device_index(0, i)
					self.input_devices[info.get('name')] = info.get('index')
					input_device_cb.addItem(info.get('name'))
		input_device_cb.move(90,30)
		i = 0
		for device, index in self.input_devices.items():
			if device == self.parent.configuration.input_device:
				input_device_cb.setCurrentIndex(i)

				break
			i += 1

		output_device = QtWidgets.QLabel(self)
		output_device.setText("Output device:")
		output_device.move(0,80)
		output_device_cb = QComboBox(self)
		output_device_cb.activated[str].connect(self.save_output_device)

		self.output_devices = {}
		for i in range(0, num_devices):
				if (p.get_device_info_by_host_api_device_index(0, i).get('maxOutputChannels')) > 0:
					info  = p.get_device_info_by_host_api_device_index(0, i)
					self.output_devices[info.get('name')] = info.get('index')
					output_device_cb.addItem(info.get('name'))
		output_device_cb.move(130,80)
		i = 0
		for device, index in self.output_devices.items():
			if device == self.parent.configuration.output_device:
				output_device_cb.setCurrentIndex(i)
				break
			i += 1
		

		save_button = QtWidgets.QPushButton(self.central_widget)
		save_button.setGeometry(QtCore.QRect(500, 440, 80, 40))
		save_button.setText("Save")
		save_button.clicked.connect(self.save)

		discard_button = QtWidgets.QPushButton(self.central_widget)
		discard_button.setGeometry(QtCore.QRect(600, 440, 80, 40))
		discard_button.setText("Discard")
		discard_button.clicked.connect(self.close)

		overwrite_cb = QCheckBox("Preguntar antes de borrar un audio sin guardar ", self)
		overwrite_cb.clicked.connect(lambda:self.save_overwrite(overwrite_cb))
		overwrite_cb.setChecked(self.parent.configuration.overwrite)
		overwrite_cb.move(0, 50)
		overwrite_cb.resize(400,40)

		cache_cb = QCheckBox("Eliminar caché al cerrar el programa ", self)
		cache_cb.clicked.connect(lambda:self.save_cache_on_close(cache_cb))
		cache_cb.setChecked(self.parent.configuration.delete_cache_on_close)
		cache_cb.move(0, 100)
		cache_cb.resize(400,40)

		self.retranslate_ui()
		QtCore.QMetaObject.connectSlotsByName(self)

	def retranslate_ui(self,):
		_translate = QtCore.QCoreApplication.translate
		self.setWindowTitle(_translate("Configuration", "Configuration"))

	def save_channel(self,text):
		self.parent.configuration.channel = text

	def save_input_device(self, text):
		self.parent.configuration.input_device = text
		self.parent.configuration.input_device_index = self.input_devices.get(text)

	def save_overwrite(self, cb):
		self.parent.configuration.overwrite = cb.isChecked()

	def save_output_device(self, text):
		self.parent.configuration.output_device = text
		self.parent.configuration.output_device_index = self.output_devices.get(text)

	def save_cache_on_close(self,cb):
		self.parent.configuration.delete_cache_on_close = cb.isChecked()

	def save(self):
		self.parent.configuration.write_configuration()
		self.close()