var threshold_chunks = [];


String.prototype.toMMSSmm = function () {
    var ms_num = parseInt(this, 10); // don't forget the second param
    var minutes   = Math.floor(ms_num / 60000);
    var seconds = Math.floor((ms_num - (minutes * 60000)) / 1000);
	var ms = Math.floor(ms_num - (minutes * 60000) - (seconds * 1000));

    if (minutes < 10) {minutes = "0"+minutes;}
	if (seconds < 10) {seconds = "0"+seconds;}

	if (ms < 100) {
		ms = "0"+ms;
	}else if(ms < 10){
		ms = "00"+ms;
	}
    return minutes+':'+seconds+'.'+ms;
}

String.prototype.toSSmm = function () {
    var ms_num = parseInt(this, 10); // don't forget the second param
    var seconds = Math.floor(ms_num / 1000);
	var ms = Math.floor(ms_num - - (seconds * 1000));

	if (ms < 100) {
		ms = "0"+ms;
	}else if(ms < 10){
		ms = "00"+ms;
	}    return seconds+'.'+ms;
}

function renderBars(bars){
	if (!drawing) {
		drawing = true;
		window.requestAnimationFrame(() => {
			canvasContext.clearRect(0, 0, width, height);
			bars.forEach((bar, index) => {
				if(threshold_chunks.includes(index)){
					canvasContext.fillStyle = `rgb(255,0,0)`;
				}else{
					canvasContext.fillStyle = `rgb(${bar * 3},100,222)`;
				}
				canvasContext.fillRect((index * (barWidth + barGutter)), halfHeight, barWidth, (halfHeight * (bar / 100)));
				canvasContext.fillRect((index * (barWidth + barGutter)), (halfHeight - (halfHeight * (bar / 100))), barWidth, (halfHeight * (bar / 100)));
			});
			drawing = false;
		});
	}
}

// Calculate the average volume
function getAverageVolume(array){
	var length = array.length;
	
	var values = 0;
	var i = 0;
	
	for (; i < length; i++) {
		values += array[i];
	}
	var aux = values / length;
	if (aux === 0){
		return 1;
	}
	return aux;
	
}

function prepareDataBars(array){
	if(array){

		var repr = new Uint8Array(array.buffer);
		bars.push(getAverageVolume(repr));
	}

	if (bars.length <= Math.floor(width / (barWidth + barGutter))) {
		renderBars(bars);
	} else {
		if(array){
			advance_threshold_chunk()
		}
		renderBars(bars.slice(bars.length - Math.floor(width / (barWidth + barGutter))), bars.length);
	}
}

function advance_threshold_chunk(){
	threshold_chunks.forEach((number, index) => {
		if(!number){
			threshold_chunks.shift();

		}else if(number>Math.floor(width / (barWidth + barGutter))){
			threshold_chunks[index] = Math.floor(width / (barWidth + barGutter)) ;

		}else{	
			threshold_chunks[index] -= 1;
		}
	});
}