const signals = require('signals');
const audioChunks = [];
var pyAudio;
var pyData;
var reader = new FileReader();
var recorder;
var mySignal = new signals.Signal();
var mediaRecorder;
mySignal.add(extractData);
reader.addEventListener("loadend", function() {
    pyData = reader.result;
    console.log("Datos desde node: ", new Int8Array(pyData))

    mySignal.dispatch();
});
const recordAudio = () => {
    return new Promise(resolve => {
        navigator.mediaDevices.getUserMedia({ audio: true })
        .then(stream => {
            mediaRecorder = new MediaRecorder(stream);
            mediaRecorder.addEventListener("dataavailable", event => {
                var aux =  mediaRecorder.stream.getAudioTracks();
                audioChunks.push(event.data);
            });
            const start = () => {
                mediaRecorder.start();
            };  
            const stop = () => {
                return new Promise(resolve => {
                    mediaRecorder.addEventListener("stop", () => {
                        const audioBlob = new Blob(audioChunks);
                        const audioUrl = URL.createObjectURL(audioBlob);
                        const audio = new Audio(audioUrl);
                        pyAudio = audioBlob
                        const play = () => {
                            audio.play();
                        };     
                        resolve({ audioBlob, audioUrl, play });
                    });                   
                    mediaRecorder.stop();
                });
            };          
            resolve({ start, stop });
        });
    });
};
function record(){
    (async () => {
        recorder = await recordAudio();
        recorder.start();
        
        return recorder;
    })();
}

function playRecord(){
    var audio;
    (async () => {
        audio = await recorder.stop();
        audio.play();
        reader.readAsArrayBuffer(pyAudio);
        audioChunks.pop();
    })();   
}

function extractData(){
    pyData = new Int8Array(pyData);
    var python = require('child_process').spawn('python3', ['./send.py',pyData]);
    python.stdout.on('data',function(data){
        console.log("Grabado en node pasado a Python: ",data.toString('utf8'));
    });
    python.stderr.on('data', function(data){
        console.log("Error: ",data.toString('utf8'));
    });
}

function recordPython(){
    var python = require('child_process').spawn('python3', ['./record.py',"1"]);
    python.stdout.on('data',function(data){
        console.log("Graba python: ",data.toString('utf8'));
    });
    python.stderr.on('data', function(data){
        console.log("Error: ",data.toString('utf8'));
    });
}

function playPython(){
    var python = require('child_process').spawn('python3', ['./record.py',"2"]);
    python.stdout.on('data',function(data){
        console.log("Reproduce python: ",data.toString('utf8'));
    });

}
