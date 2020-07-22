import pyaudio
import wave
import sys
import threading
import wavefront_ui as wafr
import time
import numpy as np
from plotRepresentation import *
from analyzer import *
from PyQt5 import QtCore,QtGui, QtWidgets
from PyQt5.QtWidgets import *
from configuration import *
class PlayAudio(QObject):
	finish_record_thread = QtCore.pyqtSignal(list)
	finish_play_thread = QtCore.pyqtSignal()

	def __init__ (self,file, canvas):
		QtCore.QThread.__init__(self)
		self.file = file
		self.canvas = canvas
		self.analyzer = None
		self.configuration = Configuration()

	def play_audio(self):
		ticks = time.time()
		time_chunk = None
		paused = False
		CHUNK = 2048
		THRESHOLD = 600
		wf = wave.open(self.file, 'rb')
		p = pyaudio.PyAudio()
		stream = p.open(format=pyaudio.paInt16,
						channels=wf.getnchannels(),
						rate=wf.getframerate(),
						output=True,
						frames_per_buffer=CHUNK,
						output_device_index=self.configuration.output_device_index)
		data = wf.readframes(CHUNK)
		processed_data = np.fromstring(data,np.int16)
		processed_data,data_analyzer = self.change_data(processed_data)
		while len(processed_data) > 0 :

			if len(processed_data) != CHUNK:
				CHUNK = len(processed_data)

			while wafr.is_paused_playing:
				paused = True

			if wafr.stop:
				break

			if  paused:
				paused = False
				ticks = time.time() - time_chunk
			else:
				time_chunk = time.time() - ticks 
			if len(self.analyzer.chunk_queue) < self.analyzer.window_size:
				self.analyzer.chunk_queue.append(np.mean(data_analyzer))
				
			else:
				self.prepare_analyzer(data_analyzer,THRESHOLD)
				myThread = threading.Thread(target = self.analyzer.run)
				myThread.start()
			
			self.canvas.plot.plot(processed_data,time_chunk, CHUNK)
			stream.write(data)
			
			data = wf.readframes(CHUNK)
			processed_data = np.fromstring(data,np.int16)
			processed_data,data_analyzer = self.change_data(processed_data)

		wafr.is_playing = False
		stream.stop_stream()
		stream.close()
		p.terminate()
		self.finish_play_thread.emit()

	def prepare_analyzer(self,processed_data, threshold):
		self.analyzer.chunk_queue.append(np.mean(processed_data))
		self.analyzer.threshold = threshold
		self.analyzer.canvas = self.canvas
		self.analyzer.index += 1

	def record_audio(self):
		info = self.get_input_info(self.configuration.input_device_index)
		ticks = time.time()
		time_chunk = 0
		CHANNELS = 2
		CHUNK = 1024
		paused = False
		p = pyaudio.PyAudio()	 
		THRESHOLD = 100
		self.canvas.configuration.channel = 2
		stream = p.open(format=pyaudio.paInt16, channels=CHANNELS,
						rate=int(info.get('defaultSampleRate')), input=True,
						frames_per_buffer=CHUNK, input_device_index=self.configuration.input_device_index)
		frames = []
		while wafr.is_recording:
			while wafr.is_paused_recording:
				paused = True
			if wafr.stop:
				break
			if  paused:
				paused = False
				ticks = time.time() - time_chunk
			else:
				time_chunk = time.time() - ticks 
			data = stream.read(CHUNK,exception_on_overflow = False)
			processed_data = np.fromstring(data,np.int16)
			processed_data,data_analyzer = self.change_data(processed_data)
			frames.append(data)
			if len(self.analyzer.chunk_queue) < self.analyzer.window_size:
				self.analyzer.chunk_queue.append(np.mean(data_analyzer))	
			else:
				self.prepare_analyzer(data_analyzer, THRESHOLD)
				myThread = threading.Thread(target = self.analyzer.run)
				myThread.start()
			self.canvas.plot.plot(processed_data,time_chunk, CHUNK )

		if wafr.stop:
			self.finish_record_thread.emit([])
		else:		
			stream.stop_stream()
			stream.close()
			p.terminate()
			data = []
			data.append(CHANNELS)
			data.append(p.get_sample_size(pyaudio.paInt16))
			data.append(int(info.get('defaultSampleRate')))
			data.append(b''.join(frames))
			self.finish_record_thread.emit(data)

	def change_data(self,data):
		processed_data = []
		data_analyzer = []
		if self.canvas.configuration.channel == 0:
			for i in range(0,len(data)):
				if i%2 == 0:
					processed_data.append(float(data[i]))
					data_analyzer.append(abs(float(data[i])))
			return np.array(processed_data),np.array(data_analyzer)
		elif self.canvas.configuration.channel == 1:
			for i in range(0,len(data)):
				if i%2 == 1:
					processed_data.append(float(data[i]))
					data_analyzer.append(abs(float(data[i])))
			return np.array(processed_data),np.array(data_analyzer)
		else:
			for i in range(0,len(data),2):
				processed_data.append((float(data[i]) + float(data[i + 1]))/2)
				data_analyzer.append((abs(float(data[i])) + abs(float(data[i + 1])))/2)
			return np.array(processed_data),np.array(data_analyzer)
	def get_input_info(self, index):
		p = pyaudio.PyAudio()
		info = p.get_host_api_info_by_index(0)
		num_devices = info.get('deviceCount')
		self.input_devices = {}
		for i in range(0, num_devices):
			info = p.get_device_info_by_host_api_device_index(0, i)
			if info.get('maxInputChannels') > 0 :
				return info
