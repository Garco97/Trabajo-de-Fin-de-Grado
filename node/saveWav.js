const fs = require('fs');

module.exports = {
    writeWavFile:function (chunks, path, isRaw){
        var raw = Buffer.concat(chunks);
        if(isRaw){
            fs.writeFileSync(path,raw);
        }else{
            var header = audioBufferToWav(raw)
            var wav = Buffer.concat([Buffer.from(header),raw]);
            fs.writeFileSync(path,wav);
        }
    }
};

function audioBufferToWav  (buffer) {
    var numChannels = 1
    var sampleRate = 44100
    var format = 3
    var bitDepth = 32
    var result = buffer
    return encodeWAV(result, format, sampleRate, numChannels, bitDepth)
}

function encodeWAV  (samples, format, sampleRate, numChannels, bitDepth) {
    var bytesPerSample = bitDepth / 8
    var blockAlign = numChannels * bytesPerSample
    
    var buffer = new ArrayBuffer(44 )
    var view = new DataView(buffer)
    
    /* RIFF identifier */
    writeString(view, 0, 'RIFF')
    /* RIFF chunk length */
    view.setUint32(4, 36 + samples.length * bytesPerSample, true)
    /* RIFF type */
    writeString(view, 8, 'WAVE')
    /* format chunk identifier */
    writeString(view, 12, 'fmt ')
    /* format chunk length */
    view.setUint32(16, 16, true)
    /* sample format (raw) */
    view.setUint16(20, format, true)
    /* channel count */
    view.setUint16(22, numChannels, true)
    /* sample rate */
    view.setUint32(24, sampleRate, true)
    /* byte rate (sample rate * block align) */
    view.setUint32(28, sampleRate * blockAlign, true)
    /* block align (channel count * bytes per sample) */
    view.setUint16(32, blockAlign, true)
    /* bits per sample */
    view.setUint16(34, bitDepth, true)
    /* data chunk identifier */
    writeString(view, 36, 'data')
    /* data chunk length */
    view.setUint32(40, samples.length * bytesPerSample, true)
    return buffer
}

function writeString (view, offset, string) {
    for (var i = 0; i < string.length; i++) {
        view.setUint8(offset + i, string.charCodeAt(i))
    }
}

