const WaveSurfer = require('wavesurfer.js');
var wavesurfer;
var playing = false;

function createWaveSurfer(){
    wavesurfer = WaveSurfer.create({
        container: "#waveform",
        waveColor: "blue",
        progressColor: "red"
    });
    wavesurfer.load('http://ia902606.us.archive.org/35/items/shortpoetry_047_librivox/song_cjrg_teasdale_64kb.mp3');
}

function processing(){
    var python = require('child_process').spawn('python', ['./hello.py']);
    python.stdout.on('data',function(data){
        console.log("data: ",data.toString('utf8'));
    });
}









