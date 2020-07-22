const { ipcRenderer } = require('electron')

var threshold_input;
var window_input;
var percentage_input;


var CHUNK_SIZE;
var FILE = undefined;
var RECORD_FILE = undefined;
var TIME;
var ACTION;
var MEASURE_OPTIONS;
var BARS;
var ONLY_RECORD;

var THRESHOLD;
var WINDOW;
var PER_WINDOW;
var MEASURE;

// UI Elements
var canvas;
var recordButton;
var playButton;
var audioPlayer;
var saveButton;
var stopButton;

var estadoLb;
var playLb;
var saveLb;
// Constants recording
var audioContext;
var scriptProcessor;
var audioContextPlaying;
var scriptProcessorPlaying;

var chunks = [];

var startFrontendTime;
var endFrontendTime;
var startFrontendWZTime;
var endFrontendWZTime;
var startSendingTime;
var firstFrontendChunk;
var gotFirstFrontendChunk;

var haveSendingTime = false;

// Variables for recording
var input = null;
var inputPlaying = null;
var source;
var stream_dest;

//When the audio is loaded, the proccesing starts
var recorder = null;
var recording = null;
var isRecording = false;
var audioComesFromRecord = false;

var nChunksCaptured = 0;

var chunk_times = [];

var first_chunk_threshold = 0;

var audio_comes_from_file = false;

// Variables for playing
var isPlaying = false; //Mira si se esta reproduciendo
var isStarted = false; //Compruea si ya se ha empezado a reproducir el audio
var record = false; //Comprueba si el audio viene de grabacion
var play = false; //Comprueba si el audio viene de fichero

// Variables para la representación del waveform
var plotRecordAnalyser;
var plotPlayAnalyser

var processingExecuted = false;
// Canvas variables
const barWidth = 2;
const barGutter = 3;
var canvasContext;
var bars = [];
var width = 0;
var height = 0;
var halfHeight = 0;
var drawing = false;


function sendThresholdParameters(){
	ipcRenderer.send('asynchronous-message', "Thresh _" + threshold_input.value + "_" + window_input.value + "_" + percentage_input.value +"_\0");
}

function saveRecordingFile(){
	ipcRenderer.send('asynchronous-message', "save");
	bars = []
	chunks = []
	stopAll(false);
}
// Start recording
function startRecording(){
	recordButton.disabled = false;
	playButton.disabled = true;
	stopButton.disabled = true;
	fileChooser.disabled = true;
	saveButton.disabled = true;
	if(typeof FILE !== "string"){
		FILE = null;
	}
	if(!record){
		bars = [];
		chunks = [];
		sendThresholdParameters();
	}
	startFrontendTime = (new Date()).getTime();
	recorder.start();
	record = true;
	play = false;	
}

// Stop recording
function stopRecording(){
	recorder.stop();	
}

// Toggle the recording button
function toggleRecording(){
	if (isRecording) {
		stopRecording();
	} else {
		startRecording();
	}
}
// Play the audio
async function playAudio() {
	playButton.disabled = false;
	stopButton.disabled = true;
	recordButton.disabled = true;
	fileChooser.disabled = true;
	saveButton.disabled = true;
	if(FILE && !isStarted && !record){
		sendThresholdParameters();
		chunk_times = [];
		thresh_area.value = "";
		bars = []
		play = true;
		audio_comes_from_file = true;
		isStarted = true;
		audioPlayer.setAttribute('src', FILE);
	}else{
		if(audioComesFromRecord){	
			if(!isStarted){
				recording = URL.createObjectURL(new Blob(chunks, { 'type' : 'audio/wav; codecs=opus' }));
				audioPlayer.setAttribute('src', recording);
			}
			audioPlayer.play();
			isPlaying = true;
			isStarted = true;
			estadoLb.innerHTML = "Estado: Reproduciendo grabación";
			playLb.innerHTML = "Fichero de reproducción: Ninguno"
			saveLb.innerHTML = "Fichero de grabación: Predeterminado"
		}else{
			audioPlayer.play();
			isPlaying = true;
			estadoLb.innerHTML = "Estado: Reproducción reanudada";
			playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE:"Ninguno");
			saveLb.innerHTML = "Fichero de grabación: " +(RECORD_FILE ? RECORD_FILE:"Predeterminado");
		}
	}
}

// Stop the play
function pauseAudio(){
	audioPlayer.pause();
	isPlaying = false;
	if(audioComesFromRecord){
		playButton.disabled = false;
		recordButton.disabled = false;
		saveButton.disabled = false;
		stopButton.disabled = false;
		fileChooser.disabled = true;
		estadoLb.innerHTML = "Estado: Reproducción de grabación pausada";
		playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE:"Ninguno");
		saveLb.innerHTML = "Fichero de grabación: " +(RECORD_FILE ? RECORD_FILE:"Predeterminado");
	}else{
		playButton.disabled = false;
		stopButton.disabled = false;
		recordButton.disabled = true;
		fileChooser.disabled = true;
		saveButton.disabled = true;
		estadoLb.innerHTML = "Estado: Reproducción pausada";
		playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE:"Ninguno");
		saveLb.innerHTML = "Fichero de grabación: " +(RECORD_FILE ? RECORD_FILE:"Predeterminado");
	}	
}

// Toggle the play button
function togglePlay(){
	if (isPlaying) {
		pauseAudio();
	} else {
		playAudio();
	}
}
//Stop
function stopAll(forced=true){
	if(isRecording){
		recorder.stop()	
	}
	if(isPlaying){
		audioPlayer.stop();
	}
	chunks = [];
	if(isStarted || (forced && !audio_comes_from_file)){
		ipcRenderer.send('asynchronous-message', "END");
	}
	isPlaying = false; 
	isRecording = false; 
	isStarted = false; 
	audioComesFromRecord = false;
	play = false; 
	record = false;
	bars = [];
	renderBars(bars);
	fileChooser.value = ""
	FILE = undefined;
	file = undefined
	threshold_chunks = [];
	chunk_times = [];
	thresh_area.value = "";
	estadoLb.innerHTML = "Estado: Detenido";
	playLb.innerHTML = "Fichero de reproducción: Ninguno"
	saveLb.innerHTML = "Fichero de grabación: Predeterminado"
	audio_comes_from_file = false;
	recordButton.disabled = false;
	fileChooser.disabled = false;
	playButton.disabled = true;
	saveButton.disabled = true;
	stopButton.disabled = true;
	ipcRenderer.send('asynchronous-message', "RESET");
}


// Process the microphone input
function processInputMeasure(audioProcessingEvent){
	if(!haveSendingTime && isRecording){
		haveSendingTime = true;
		startSendingTime = (new Date()).getTime();
	}
	chunk = audioProcessingEvent.inputBuffer.getChannelData(0);	
	if(!gotFirstFrontendChunk && isRecording){
		firstFrontendChunk = (new Date()).getTime();
		gotFirstFrontendChunk = true;
	}
	if (isRecording){
		streamRecordProcessing(chunk);
	}		
}

function processPlayedInputMeasure(audioProcessingEvent){
	if(!haveSendingTime && isPlaying){
		haveSendingTime = true;
		startSendingTime = (new Date()).getTime();
	}
	chunk = audioProcessingEvent.inputBuffer.getChannelData(0);	
	if(!gotFirstFrontendChunk && isPlaying){
		firstFrontendChunk = (new Date()).getTime();
		gotFirstFrontendChunk = true;
	}
	if(isPlaying){
		streamPlayProcessing(chunk);
	}
}

function processInput(audioProcessingEvent){
	chunk = audioProcessingEvent.inputBuffer.getChannelData(0);	
	if (isRecording){
		chunk_times.push((new Date()).getTime())
		streamRecordProcessing(chunk);				
	}
}

function processPlayedInput(audioProcessingEvent){
	chunk = audioProcessingEvent.inputBuffer.getChannelData(0);	
	if (isPlaying){
		chunk_times.push((new Date()).getTime())
		streamPlayProcessing(chunk);		
	}
}

function streamRecordProcessing(chunks){
	if(MEASURE){
		if(!ONLY_RECORD){
			ipcRenderer.send('asynchronous-message', chunks);	
		}else{
			nChunksCaptured ++;		
		}
		if(BARS){
			const array = new Uint8Array(plotRecordAnalyser.frequencyBinCount);
			plotRecordAnalyser.getByteFrequencyData(array);
			prepareDataBars(array);
		}
	}else{
		ipcRenderer.send('asynchronous-message', chunks);
		const array = new Uint8Array(plotRecordAnalyser.frequencyBinCount);
		plotRecordAnalyser.getByteFrequencyData(array);
		prepareDataBars(array);
	}
}

function streamPlayProcessing(chunks){
	if(MEASURE){
		if(!ONLY_RECORD){
			ipcRenderer.send('asynchronous-message', chunks);	
		}else{
			nChunksCaptured ++;	
		}
		if(BARS){
			const array = new Uint8Array(plotPlayAnalyser.frequencyBinCount);
			plotPlayAnalyser.getByteFrequencyData(array);
			prepareDataBars(array);
		}
	}else{
		if(!record){
			ipcRenderer.send('asynchronous-message', chunks);
			const array = new Uint8Array(plotPlayAnalyser.frequencyBinCount);
			plotPlayAnalyser.getByteFrequencyData(array);
			prepareDataBars(array);
		}
	}
}

function prepareFileProccesing() {
	source = audioContextPlaying.createMediaElementSource(audioPlayer);
	stream_dest = audioContextPlaying.createMediaStreamDestination();
	source.connect(stream_dest); 
	source.connect(audioContextPlaying.destination)
	if(!MEASURE){
	}
	inputPlaying = audioContextPlaying.createMediaStreamSource(stream_dest.stream);
	inputPlaying.connect(scriptProcessorPlaying);
	if(!MEASURE || BARS){
		inputPlaying.connect(plotPlayAnalyser);
	}
	scriptProcessorPlaying.connect(audioContextPlaying.destination);
	if (MEASURE){
		scriptProcessorPlaying.onaudioprocess = processPlayedInputMeasure;
	}else{
		scriptProcessorPlaying.onaudioprocess = processPlayedInput;
	}
}


function getFile(f){
	FILE = f;
	playButton.disabled = false;
	stopButton.disabled = false;
	recordButton.disabled = true;
	fileChooser.disabled = true;
	saveButton.disabled = true;
	estadoLb.innerHTML = "Estado: Fichero cargado";
	playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE:"Ninguno");
	saveLb.innerHTML = "Fichero de grabación: " +(RECORD_FILE ? RECORD_FILE:"Predeterminado");
}

/*
* Block that initialize the entire system
*/
function getArguments(args){
	let {CHUNK_SIZE: cs, FILE: f, TIME: t, ACTION: a, MEASURE_OPTIONS: mo, THRESHOLD: th, WINDOW: w, PER_WINDOW:p} = args
	CHUNK_SIZE = cs;
	ACTION = a;
	FILE = f !== undefined ? f: undefined;
	if(ACTION === "record"){
		RECORD_FILE = FILE;
		FILE = undefined;
	}
	TIME = t !== undefined ? parseFloat(t) : null ;
	
	THRESHOLD = th !== undefined ? parseInt(th) : 30 ;
	WINDOW = w !== undefined ? parseInt(w) : 20 ;
	PER_WINDOW = p !== undefined ? parseInt(p) : 50 ;

	MEASURE_OPTIONS = mo;
	if(ACTION){
		MEASURE = ACTION.search("measure") !== -1 ? true:false;
	}
	if (MEASURE && MEASURE_OPTIONS){
		if ( MEASURE_OPTIONS.search("bars") !== -1){	
			BARS = true;
		}
		if (MEASURE_OPTIONS.search("onlyrecord") !== -1){
			ONLY_RECORD = true;
		}
	}
}

function initializeInterface(){
	if (!window.AudioContext) {
		setMessage('Your browser does not support window.Audiocontext. This is needed for this demo to work. Please try again in a differen browser.');
	}
	canvas = document.querySelector('.js-canvas');
	recordButton = document.querySelector('.js-record');
	playButton = document.querySelector('.js-play');
	stopButton = document.querySelector('.js-stop');
	saveButton = document.querySelector('.js-save');
	canvasContext = canvas.getContext('2d');
	threshold_input = document.getElementById("threshold");
	window_input = document.getElementById("window");
	percentage_input = document.getElementById("percentage");
	estadoLb = document.getElementById("estado_lb");
	playLb = document.getElementById("play_lb");
	saveLb = document.getElementById("save_lb");
	thresh_area = document.getElementById("textarea");
	threshold_input.value = THRESHOLD;
	window_input.value = WINDOW;
	percentage_input.value = PER_WINDOW;

}
function requestMicrophoneAccess(){ 
	if (navigator.mediaDevices) {
		navigator.mediaDevices.getUserMedia({audio: true}).then(stream => {
			setAudioStream(stream);		
			setupWaveform();
		
			console.log("Tengo microfono");
		}, error => {
			console.log('Something went wrong requesting the userMedia. <br/>Please make sure you\'re viewing this demo over https.');
			console.log(error);
			setupWaveform();

		});
	} else {
		console.log('Your browser does not support navigator.mediadevices. <br/>This is needed for this demo to work. Please try again in a differen browser.');
		setupWaveform();

	}
}
// Set all variables which needed the audio stream
function setAudioStream(stream){
	input = audioContext.createMediaStreamSource(stream);
	recorder = new window.MediaRecorder(stream);
	console.log(input +" aqui estoy, cogiendo el strem")
	setRecorderEvents(); 

};

// Setup the recorder actions
function setRecorderEvents(){
	recorder.onstart = async function(){
		estadoLb.innerHTML = "Estado: Grabando";
		playLb.innerHTML = "Fichero de reproducción: Ninguno"
		saveLb.innerHTML = "Fichero de grabación: " + (FILE ? FILE:"Predeterminado");
		isRecording = true;
	};
	recorder.ondataavailable = function(){
		chunks.push(event.data);
	};
	
	recorder.onstop = async function(){
		endFrontendTime = (new Date()).getTime();
		isRecording = false;
		recording = URL.createObjectURL(new Blob(chunks, { 'type' : 'audio/wav; codecs=opus' }));
		audioPlayer.setAttribute('src', recording);
		audioComesFromRecord = true;
		estadoLb.innerHTML = "Estado: Grabación finalizada";
		playLb.innerHTML = "Fichero de reproducción: Ninguno"
		saveLb.innerHTML = "Fichero de grabación: " + (FILE ? FILE:"Predeterminado");
		if(!MEASURE){
			playButton.disabled = false;
			recordButton.disabled = false;
			stopButton.disabled = false;
			saveButton.disabled = false;
			fileChooser.disabled = true;
		}else{
			var times = "F".concat(startFrontendTime.toString() + ";" + endFrontendTime.toString() + ";" + startSendingTime.toString() + ";" +
			firstFrontendChunk.toString() + ";" + nChunksCaptured.toString() + ";" + startFrontendWZTime.toString() + ";" + 
			endFrontendWZTime.toString())
			ipcRenderer.send('asynchronous-message', times);
			ipcRenderer.send('asynchronous-message', "END");		
		}
	};
}
// Setup the canvas to draw the waveform
function setupWaveform(){
	canvasContext = canvas.getContext('2d');
	width = canvas.offsetWidth;
	height = canvas.height;
	halfHeight = height / 2;
	
	canvasContext.canvas.width = width;
	canvasContext.canvas.height = height; 
	console.log(input);
	if(input){
		console.log("Tengo microfono, proceso audio");
		input.connect(scriptProcessor);
	}
	if(!MEASURE || BARS){
		if(input){
			console.log("Microfono y represento audio");
			input.connect(plotRecordAnalyser);
		}
	}
	scriptProcessor.connect(audioContext.destination);
	if(MEASURE){
		scriptProcessor.onaudioprocess = processInputMeasure;	
	}else{
		scriptProcessor.onaudioprocess = processInput;
	}
}
function initializeEventListeners(){
	setAudioPlayer();
	setButtonListeners();
}
function setButtonListeners(){
	recordButton.addEventListener('mouseup', toggleRecording);
	playButton.addEventListener('mouseup', togglePlay);
	saveButton.addEventListener('mouseup', saveRecordingFile);
	stopButton.addEventListener('mouseup', stopAll)
}

function setAudioPlayer(){
	audioPlayer.onloadeddata = function(){
		if(!record && play && !audioComesFromRecord ){
			//prepareFileProccesing();
			estadoLb.innerHTML = "Estado: Reproduciendo";
			playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE: "Ninguno");
			saveLb.innerHTML = "Fichero de grabación: Predeterminado"
			if(!MEASURE){
				isPlaying = true;
				setTimeout(function(){
					audioPlayer.play();
				},100)
			}else{
				startFrontendWZTime = (new Date()).getTime();
				isPlaying = true;
				setTimeout(function(){
					startFrontendTime = (new Date()).getTime();
					audioPlayer.play();
				},400)
			}
		}
	}
	audioPlayer.onended = function(){
		playButton.disabled = false;
		stopButton.disabled = false;
		fileChooser.disabled = true;
		recordButton.disabled = true;
		saveButton.disabled = true;
		if(!MEASURE){
			estadoLb.innerHTML = "Estado: Reproducción finalizada";
			playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE:"Ninguno");
			saveLb.innerHTML = "Fichero de grabación: Predeterminado"
			isPlaying = false;
			isStarted = false;
			
			if(!audioComesFromRecord){
				ipcRenderer.send('asynchronous-message', "END");			
			}else{
				recordButton.disabled = false;
				saveButton.disabled = false;
			}
		}else{
			endFrontendTime = (new Date()).getTime();
			setTimeout(function(){
				isPlaying = false
				endFrontendWZTime = (new Date()).getTime();
				var times = "F".concat(startFrontendTime.toString() + ";" + endFrontendTime.toString() + ";" + startSendingTime.toString() + ";" +
				firstFrontendChunk.toString() + ";" + nChunksCaptured.toString() + ";" + startFrontendWZTime.toString() + ";" + 
				endFrontendWZTime.toString())
				ipcRenderer.send('asynchronous-message', times);
				ipcRenderer.send('asynchronous-message', "END");		
			},400)
		}
	}
}


function initializeAll(args){
	audioPlayer = document.createElement("AUDIO");
	if(args){
		getArguments(args);
	}
	initializeInterface();
	initializeRecordContext();
	requestMicrophoneAccess();

	prepareFileProccesing();
	initializeEventListeners()
	disableButtons();
}
function initializeRecordContext(){
	audioContext =   new AudioContext();
	scriptProcessor = audioContext.createScriptProcessor(CHUNK_SIZE, 1, 1);
	audioContextPlaying = new AudioContext();
	scriptProcessorPlaying = audioContextPlaying.createScriptProcessor(CHUNK_SIZE, 1, 1);
	if (!MEASURE || BARS){
		plotPlayAnalyser = audioContextPlaying.createAnalyser();
		plotRecordAnalyser = audioContext.createAnalyser();
		plotPlayAnalyser.smoothingTimeConstant = 0.3;
		plotPlayAnalyser.fftSize = 8192;
		plotRecordAnalyser.smoothingTimeConstant = 0.3;
		plotRecordAnalyser.fftSize = 8192;
	}	
}

function disableButtons(){
	playButton.disabled = true;
	recordButton.disabled = true;
	stopButton.disabled = true;
	saveButton.disabled = true;
	fileChooser.disabled = true;	
}

function enableButtons(){
	playButton.disabled = false;
	recordButton.disabled = false;
	stopButton.disabled = false;
	saveButton.disabled = false;
	fileChooser.disabled = false;	
}

/*
*End of initialize block
*/

ipcRenderer.on('ping' , function(event , args){ 
	initializeAll(args);
	disableButtons();
	setTimeout(function() {
		if (ACTION){
			switch(ACTION){
				case "play":
				playAudio();
				break;
				case "record":
				startRecording();
				setTimeout(function() {
					stopRecording();
				}
				, TIME * 1000);
				break;

				case "measure-record":
				disableButtons();
				startFrontendWZTime = 0;
				endFrontendWZTime = 0;
				startRecording();
				setTimeout(function() {
					stopRecording();
					saveRecordingFile();
				}
				, TIME * 1000);
				break;
				case "measure-play":
				disableButtons();

				playAudio();
				break;
			}
		}else{
			estadoLb.innerHTML = "Estado: Iniciado";
			playLb.innerHTML = "Fichero de reproducción: " + (FILE ? FILE:"Ninguno");
			saveLb.innerHTML = "Fichero de grabación: " +(RECORD_FILE ? RECORD_FILE:"Predeterminado");
			recordButton.disabled = false;
			fileChooser.disabled = false;
		}
	},1000);
});

ipcRenderer.on('threshold', function(event, args){
	var delay_warn = (new Date()).getTime();
	aux = args.split("_");
	//! Mensajes del threshold
	if(aux.length >= 6){
		var last_sample = parseInt(aux[1]);
		var last_chunk = parseInt(aux[2]);
		var first_sample = parseInt(aux[3]);
		var first_chunk =  parseInt(aux[4]);
		var delay = delay_warn - chunk_times[first_chunk]
		var start_time = ((first_chunk*CHUNK_SIZE + first_sample)/44.1).toFixed(0);
		var end_time = ((last_chunk*CHUNK_SIZE + last_sample)/44.1).toFixed(0);
		var total_time = end_time - start_time;
		for(var i=first_chunk;i<=last_chunk;i++){
			var max_chunk_representation = Math.floor(width / (barWidth + barGutter)) 
			if(i > max_chunk_representation){
				if(!threshold_chunks.includes(max_chunk_representation)){
					threshold_chunks.push(max_chunk_representation)
				}
			}else{
				if(!threshold_chunks.includes(i)){
					threshold_chunks.push(i)
				}
			}
		}
		prepareDataBars(null)
		thresh_area.value += "["+start_time.toString().toMMSSmm()+" : "+ end_time.toString().toMMSSmm()+"] threshold superado en un " + 
		percentage_input.value + "% de valores de ventana ("+total_time.toString().toSSmm()+"s) [delay ~"+delay+"ms]\n"
	}else{
		//! Mensajes del benchmark threshold
		var chunk_number = parseInt(aux[2]);
		ipcRenderer.send('asynchronous-message', "Delay " + chunk_times[chunk_number] + "," + delay_warn + "\0");
		if(!threshold_chunks.includes(chunk_number)){
			threshold_chunks.push(chunk_number)
		}
		prepareDataBars(null)
		var delay = delay_warn - chunk_times[chunk_number]
		thresh_area.value += "Se ha superado el threshold con :" + parseFloat(aux[0]) +" en el sample: "+
		 parseInt(aux[1]) +" del chunk: " + parseInt(aux[2])+
		 " con un retraso de "+ delay + " ms\n";
	}
})