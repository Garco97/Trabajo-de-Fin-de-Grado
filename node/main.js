const process = require('process');
const {app, BrowserWindow} = require('electron');
const electron = require('electron');
const debug = require('electron-debug');
const unhandled = require('electron-unhandled');
const contextMenu = require('electron-context-menu');
const { ipcMain } = require('electron')
const fs = require('fs')
const nano = require('nanomsg');
const wav = require('./saveWav');
const { time } = require('console');


//! Protocolo de comunicación
var ack = false;
var config = false;

let win;
const COMMON_PATH ="benchmark_comms/"
// Variables de entorno
const START_TIME = process.env.START_TIME;
const CHUNK_SIZE = process.env.CHUNK_SIZE;
var FILE = process.env.FILE;
const TIME = process.env.TIME;
const ACTION = process.env.ACTION;
const MEASURE_OPTIONS = process.env.MEASURE_OPTIONS;
const TEST = process.env.TEST;
const THRESHOLD = process.env.THRESHOLD;
const WINDOW = process.env.WINDOW;
const PER_WINDOW = process.env.PER_WINDOW;

if(!CHUNK_SIZE ){
  console.log("Faltan variables de entorno CHUNK_SIZE");
  process.exit();
}
var C_PROCESSING;
var ONLY_RECORD;
var MEASURE = ACTION === "measure-record" || ACTION === "measure-play" ? true : false; 
if(MEASURE && MEASURE_OPTIONS){
  C_PROCESSING = MEASURE_OPTIONS.search("cprocessing") > -1 ? true : false;
  ONLY_RECORD = MEASURE_OPTIONS.search("onlyrecord") > -1 ? true : false;
}
var TECH = "";
if(process.env.TECH){
  TECH = process.env.TECH;
}else{
  if(MEASURE){
    if(C_PROCESSING){
      TECH = "ipc";
    }else{
      TECH = "";
    }
  }else{
    TECH = "ipc";
  }
}
const benchmarkPath = "".concat(COMMON_PATH,"benchmark_audios/",  TIME,"-",CHUNK_SIZE, "-",TECH, "-", MEASURE_OPTIONS, "-",TEST,".wav");
const originalPath = "".concat(COMMON_PATH,"original_audios/",  TIME,"-",CHUNK_SIZE, "-",TECH, "-", MEASURE_OPTIONS, "-",TEST,".wav");
const timesPath = "".concat(COMMON_PATH,"results_benchmark/",  TIME,"-",CHUNK_SIZE, "-",TECH, "-", MEASURE_OPTIONS,"-",TEST,".txt");
// Lista de Chunks
var chunksObject = [];
var buffer = Buffer.alloc(parseInt(CHUNK_SIZE)*4);
buffer.fill(0.0);



//Mediciones 
var startFrontendTime;
var endFrontendTime;
var startFrontendWZTime;
var endFrontendWZTime;
var firstFrontendChunk;

var startSendingTime;
var endSendingTime;
var firstSendingChunk;
var gotFirstSendingChunk;

var STARTED_NANO;
var startNanoTime;
var endNanoTime;
var firstNanoChunk;

var nChunksWZ;
var nChunks;


ipcMain.on('asynchronous-message', (event, arg) => {
  if (arg[0] === "F"){
    var times = arg.substring(1);
    times = times.split(";");
    startFrontendTime = times[0];
    endFrontendTime = times[1];
    startSendingTime = times[2];
    firstFrontendChunk = times[3];
    nChunksWZ = !ONLY_RECORD ? chunksObject.length : times[4];
    startFrontendWZTime = times[5];
    endFrontendWZTime = times[6];
    if(endNanoTime || !C_PROCESSING){
      printStats();
    }  
  }else if(arg === "END"){
    if(!MEASURE || C_PROCESSING){

      pair.send("END\0");
    }
  }else if(arg === "save"){
    if(!FILE){
      FILE = COMMON_PATH.concat("original_audios/test.wav");
    } 
    if(!MEASURE || C_PROCESSING){
      pair.send("END\0");
      pair.send("STORE_FILES\0")
    }else{
      wav.writeWavFile(chunksObject, FILE,false);
      wav.writeWavFile(chunksObject, COMMON_PATH.concat("benchmark_audios/ORIGINAL.wav"),true); 

    }
    if(ACTION === "measure-record"){
      win.close();
    } 
  }else if(arg === "RESET"){
    chunksObject = [];
  }else if(arg.includes("Delay")){
    var aux = arg.split(" ")
    pair2.send(aux[1])
  }else if(arg.includes("Thresh")){
    if(!MEASURE || C_PROCESSING){

      pair.send("START\0")
      var aux = arg.split(" ")
      pair.send(aux[1])
    }
  }else{
    // Coge el tiempo de recibir el primer chunk para calcular latencias
    if(!gotFirstSendingChunk){
      firstSendingChunk = (new Date()).getTime();
      gotFirstSendingChunk = true;
    }
    // Evita los chunks vacios, luego hay que procesarlos para que el audio sea el mismo      
    chunksObject.push(arg); 
    endSendingTime = (new Date()).getTime();
    // Solo se enviar por nanomsg si hay mediciones y se decide que se procese
    if(MEASURE && C_PROCESSING){
      // Captura del tiempo de inicio de envío en nanomsg
      if (!STARTED_NANO){
        startNanoTime = (new Date()).getTime();
        STARTED_NANO = true; 
      }
    }
    if(C_PROCESSING||!MEASURE){
      pair.send(arg)
    }
  }
});

function writeRawFile(){
  //Limpia todos los chunks que sean todo 0s del principio
  for(var i = 0; i < chunksObject.length ; i++){
    if(Buffer.compare(chunksObject[i],buffer) !== 0){
      chunksObject = chunksObject.slice(i);
      break;
    }
  }
  //Limpia los chunks que sean todo 0s del final
  for(var i = chunksObject.length-1; i >= 0 ; i--){
    if(Buffer.compare(chunksObject[i],buffer) !== 0){
      chunksObject = chunksObject.slice(0,i+1);
      break;
    }
  }
  //Limpia el primer chunk de 0s
  for (var i = 0; i < chunksObject[0].length ; i++) {
    if(chunksObject[0][i] !== 0){
      chunksObject[0] = chunksObject[0].slice(i);   
      break;
    }
  }
  //Limpia el ultimo chunk de 0s
  for (var i = chunksObject[chunksObject.length - 1].length - 1; i >= 0; i--) {
    if(chunksObject[chunksObject.length - 1][i] !== 0){
      chunksObject[chunksObject.length - 1] = chunksObject[chunksObject.length - 1].slice(0,i+1);
      break;
    }
  }
  nChunks = chunksObject.length;
  wav.writeWavFile(chunksObject, benchmarkPath, true);
}

function printStats(){
   if(!ONLY_RECORD){
    //*Guarda el fichero sin metadatos, para comprobar que el audio está correcto.
    writeRawFile();
    wav.writeWavFile(chunksObject, originalPath, false);
  } 
  
  //*Offsets
  //console.log("Empieza frontend: +",(startFrontendTime/1000. - START_TIME).toFixed(3));
  //console.log("Termina frontend: +",(endFrontendTime/1000. - START_TIME).toFixed(3));
  if(!ONLY_RECORD){
    //console.log("Empieza comunicación Electron: +",(startSendingTime/1000. - START_TIME).toFixed(3));
    //console.log("Termina comunicación Electron: +",(endSendingTime/1000. - START_TIME).toFixed(3));
  }
  if(C_PROCESSING){
    //console.log("Empieza nanomsg: +",(startNanoTime/1000. - START_TIME).toFixed(3));
    //console.log("Termina nanomsg: +",(endNanoTime/1000. - START_TIME).toFixed(3));
  } 

  //*Tiempos
  var frontendTime = getTime(startFrontendTime,endFrontendTime);
  var frontendThroughput = getThroughput(frontendTime);
  //console.log(frontendTime,"s en procesar frontend");
  //console.log(frontendThroughput,"KBps"); 
  if(!ONLY_RECORD){
    var sendingTime = getTime(startSendingTime,endSendingTime);
    var sendingThroughput = getThroughput(sendingTime);
    //console.log(sendingTime,"s en enviar por Electron");
    //console.log(sendingThroughput,"KBps"); 
  }
  if(C_PROCESSING){
    var nanoTime = getTime(startNanoTime,endNanoTime);
    var nanoThroughput = getThroughput(nanoTime);
    //console.log(nanoTime, "s en llegar a processor");
    //console.log(nanoThroughput,"KBps");
    
    //console.log(nChunksWZ, "chunks " + CHUNK_SIZE+ " bytes han sido enviados en",(endNanoTime-startFrontendTime)/1000.,"segundos");
    //console.log("Total:", nChunksWZ*CHUNK_SIZE*4,"bytes,",nChunksWZ*CHUNK_SIZE*4/1024., "Kilobytes");
    //console.log((nChunksWZ*CHUNK_SIZE*4/1024.)/((endNanoTime-startFrontendTime)/1000.),"KBps");
    
  }
  //console.log("\n\n\n"); 
  
  writeTimesToFile();
  win.close();
}

function writeTimesToFile(){
  var toFile = "".concat(startFrontendTime/1000. - START_TIME,"\n",endFrontendTime/1000. - START_TIME,"\n",firstFrontendChunk/1000. - START_TIME,"\n",
                         startFrontendWZTime/1000. - START_TIME,"\n",endFrontendWZTime/1000. - START_TIME,"\n");
  if(!ONLY_RECORD){
    toFile = toFile.concat(startSendingTime/1000. - START_TIME,"\n",endSendingTime/1000. - START_TIME,"\n",firstSendingChunk/1000. - START_TIME,"\n");
  }
  if(C_PROCESSING){
    toFile = toFile.concat(startNanoTime/1000. - START_TIME,"\n",endNanoTime/1000. - START_TIME,"\n",firstNanoChunk/1000. - START_TIME,"\n");
  } 
  if(!ONLY_RECORD){
    var size = 4 * (nChunks-2) * CHUNK_SIZE + (chunksObject[0].length + chunksObject[chunksObject.length - 1].length)
    var sizeWZ = nChunksWZ * CHUNK_SIZE * 4
    toFile = toFile.concat(size,"\n",sizeWZ,"\n");
  }else{
    var sizeWZ = nChunksWZ * CHUNK_SIZE * 4
    toFile = toFile.concat(sizeWZ,"\n");
  }
  toFile = toFile.concat(TEST,"\n");

  var toFile = "S ".concat(startFrontendTime*1000)
  toFile = toFile.concat("\n")
  if(!MEASURE){

    var myPath = TEST
    fs.writeFile(myPath,toFile,function (err) {
      if (err) return console.log(err);
    });
  }else{
    fs.writeFile(timesPath,toFile,function (err) {
      if (err) return console.log(err);
    });
  }
}

var addr;
var pair;
var addr2;
var pair2;

function socketBinding(){
  if (MEASURE && C_PROCESSING){
    addr = TECH + '://127.0.0.1:54272'
    pair = nano.socket('pair');
    pair.bind(addr);
    pair.setEncoding('utf8');
    pair.on('data',function(buf){
      if(!ack){
        ack = true;
        createWindow()
      }else{
      setTimeout(() => {
        printStats();
        }, 100); 
      }/* 
      endNanoTime = (new Date()).getTime();
      firstNanoChunk = parseFloat(buf)/1000.;
      if(startFrontendTime){
        printStats();
      } */
    });
  }else if(!MEASURE){
    addr = TECH + '://127.0.0.1:54272'  
    pair = nano.socket('pair');
    pair.bind(addr);
    pair.setEncoding('utf8');
    pair.on('data',function(buf){
      if(!ack){
        ack = true;
        createWindow()
      }else{
      setTimeout(() => {
          win.close()
        }, 100); 
      }
    });

    addr2 =  TECH + '://127.0.0.1:54273'
    pair2 = nano.socket('pair');
    pair2.bind(addr2);
    pair2.setEncoding('utf8');
    pair2.on('data',function(buf){
      win.webContents.send('threshold', buf )
    }); 
  }else{
    app.on('ready', createWindow)

  }
}

function getTime(start, end){
  var time = parseFloat((end - start)/1000.).toFixed(3);
  return time;
}

function getThroughput(timee){
  throughput = parseFloat((nChunksWZ * CHUNK_SIZE*4/1024.)/timee).toFixed(3);
  return throughput;
}

function createWindow () {
  debug();
  unhandled();
  contextMenu();
  
  win = new BrowserWindow({width: 1000, height: 800, webPreferences:{nodeIntegration: true}});
  
  win.loadFile('index.html') ; 
  screen = electron.screen;
  win.on('close', function() { 
    if(!MEASURE || C_PROCESSING){

      pair.send("EXIT\0");
    }
});
  win.webContents.on('did-finish-load', () => {
    if(ACTION){
      if(FILE){
        if(ACTION === "record"){
          win.webContents.send('ping',{CHUNK_SIZE, undefined, TIME, ACTION, MEASURE_OPTIONS,THRESHOLD,WINDOW,PER_WINDOW}  )
        }else if(ACTION === "play"){
          win.webContents.send('ping',{CHUNK_SIZE, FILE, TIME, ACTION, MEASURE_OPTIONS,THRESHOLD,WINDOW,PER_WINDOW}  )
          FILE = undefined;      
        }else{
          win.webContents.send('ping',{CHUNK_SIZE, FILE, TIME, ACTION, MEASURE_OPTIONS,THRESHOLD,WINDOW,PER_WINDOW}  )
        }
      }else{
        console.log("Falta FILE para ACTION: " + ACTION);
        pair.send("END\0")
          win.close()
      }
    }else{
      win.webContents.send('ping',{CHUNK_SIZE, FILE, TIME, ACTION, MEASURE_OPTIONS,THRESHOLD,WINDOW,PER_WINDOW}  )
    }
  })
}


// Creación del socket para la comunicación nanomsg
socketBinding()
//! Prueba de conexión con el servidor
if ((MEASURE && C_PROCESSING) || !MEASURE ){
  pair.send("ACK\0");  
}
/* 
if (MEASURE && C_PROCESSING){
  app.on('ready', createWindow)

} */