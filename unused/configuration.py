import json
import os.path
from os import path
class Configuration():
	def __init__(self):
		self.load_configuration()

	def load_configuration(self):
		if not self.load_cache():
			if path.exists(".configuration.json"):
				with open(".configuration.json") as file:
					data = json.load(file)
					self.channel = data["channel"]
					self.input_device = data["input_device"]
					self.output_device= data["output_device"] 
					self.overwrite = data["overwrite"]
					self.input_device_index = data["input_device_index"]
					self.output_device_index = data["output_device_index"]
					self.delete_cache_on_close = data["delete_cache_on_close"]
			else:
				self.channel = -1
				self.input_device = ""
				self.output_device = ""
				self.input_device_index = -1
				self.output_device_index = -1
				self.overwrite = False
				self.delete_cache_on_close = False
		if path.exists(".configuration.json"):
			with open(".configuration.json") as file:
				data = json.load(file)
				self.channel = data["channel"]
				self.overwrite = data["overwrite"]
				self.delete_cache_on_close = data["delete_cache_on_close"]
		else:
			self.channel = -1
			self.overwrite = False
			self.delete_cache_on_close = False


	def write_configuration(self):
		with open(".configuration.json",'w') as file:
			data = {}
			data["channel"] = self.channel
			data["overwrite"] = self.overwrite
			data["input_device"] = self.input_device
			data["input_device_index"] = self.input_device_index
			data["output_device"] = self.output_device
			data["output_device_index"] = self.output_device_index
			data["delete_cache_on_close"] = self.delete_cache_on_close
			json.dump(data, file)

	def load_cache(self):
		if path.exists(".cache.json"):
			with open(".cache.json") as file:
				data = json.load(file)
				self.input_device = data["input_device"]
				self.output_device= data["output_device"] 
				self.input_device_index = data["input_device_index"]
				self.output_device_index = data["output_device_index"] 
				return True
		else:
			return False

	def write_cache(self):
		with open(".cache.json",'w') as file:
			data = {}
			data["input_device"] = self.input_device
			data["output_device"] = self.output_device
			data["input_device_index"] = self.input_device_index
			data["output_device_index"] = self.output_device_index

			json.dump(data, file)