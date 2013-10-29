    var socket;
    var firstconnect = true,
        i2cNum  = "0x70",
	disp = [];

// Create a matrix of LEDs inside the <table> tags.
var matrixData;
for(var j=7; j>=0; j--) {
	matrixData += '<tr>';
	for(var i=0; i<8; i++) {
	    matrixData += '<td><div class="LED" id="id'+i+'_'+j+
		'" onclick="LEDclick('+i+','+j+')">'+
		i+','+j+'</div></td>';
	    }
	matrixData += '</tr>';
}
$('#matrixLED').append(matrixData);

var click = new Array();
for(var j = 7; j>=0; j--){
	click[j] = new Array();
	for(var i = 7; i>=0; i--)
	   click[j][i] = 0;
}

// Send one column when LED is clicked.
function LEDclick(i, j) {
	//alert(i+","+j+" clicked"+click[i][j]);
    //disp[i] ^= 0x1<<j;
    //disp[i+8] ^= 0x1<<(j+8);
  
    switch(click[i][j]){
	case 0:                   //1st click, turn led red
	     click[i][j] = 1;
	     $('#id'+i+'_'+j).addClass('cred');
	     disp[i+8] ^= 0x1<<(j);
	     break;
	case 1:				// 2nd click , turn led orange
	     click[i][j] +=1;
	     $('#id'+i+'_'+j).removeClass('cred');
	     $('#id'+i+'_'+j).addClass('corange');
	     disp[i] ^= 0x1<<(j);
	     break;
	case 2:				//3rd click, turn led green
	     click[i][j] += 1;
	     $('#id'+i+'_'+j).removeClass('corange');
	     $('#id'+i+'_'+j).addClass('cgreen');
	     disp[i+8] ^= 0x1<<(j);
	     break;
	case 3:				//4th click, turn led off
	     click[i][j] = 0;
	     $('#id'+i+'_'+j).removeClass('cgreen');
	     //$('#id' + i + '_' + j).addClass('cnone');
	     disp[i] ^= 0x1<<(j);
	     break;
    }

    //disp[i] ^= 0x1<<j;
    socket.emit('i2cset', {i2cNum: i2cNum, i: i, 
			     disp: '0x'+disp[i].toString(16)});

        socket.emit('i2cset', {i2cNum: i2cNum, i: (i+0.5), 
			     disp: '0x'+disp[i+8].toString(16)});
//	socket.emit('i2c', i2cNum);
    // Toggle bit on display
 //   if(disp[i]>>j&0x1 === 1) {
//	$('#id'+i+'_'+j).removeClass('color');
 //       $('#id'+i+'_'+j).addClass('on');
  //  } else {
 //       $('#id'+i+'_'+j).removeClass('on');
//	$('#id'+i+'_'+j).addClass('color');
 //   }
}

    function connect() {
      if(firstconnect) {
        socket = io.connect(null);

        // See https://github.com/LearnBoost/socket.io/wiki/Exposed-events
        // for Exposed events
        socket.on('message', function(data)
            { status_update("Received: message " + data);});
        socket.on('connect', function()
            { status_update("Connected to Server"); });
        socket.on('disconnect', function()
            { status_update("Disconnected from Server"); });
        socket.on('reconnect', function()
            { status_update("Reconnected to Server"); });
        socket.on('reconnecting', function( nextRetry )
            { status_update("Reconnecting in " + nextRetry/1000 + " s"); });
        socket.on('reconnect_failed', function()
            { message("Reconnect Failed"); });

        socket.on('matrix',  matrix);
        // Read display for initial image.  Store in disp[]
        socket.emit("matrix", i2cNum);

        firstconnect = false;
      }
      else {
        socket.socket.reconnect();
      }
    }

    function disconnect() {
      socket.disconnect();
    }

    // When new data arrives, convert it and display it.
    // data is a string of 16 values, each a pair of hex digits.
    function matrix(data) {
        var i, j;
        disp = [];
        //        status_update("i2c: " + data);
        // Make data an array, each entry is a pair of digits
        data = data.split(" ");
        //        status_update("data: " + data);
        // Every other pair of digits are Green. The others are red.
        // Ignore the red.
        // Convert from hex.
        for (i = 0; i < 8; i ++) {
            disp[i] = parseInt(data[2*i], 16);
	    disp[i+8] = parseInt(data[2*i+1], 16);
	    
        }
        //        status_update("disp: " + disp);
        // i cycles through each column
        for (i = 0; i < 8; i ++) {
            // j cycles through each bit
            for (j = 0; j < 8; j++) {
                if ((((disp[i] >> j) & 0x1) === 1)&&(((disp[i+8] >> j) | 0x0)===0)) {
		    //$('#id' + i + '_' + j).removeClass('corange');
		    //$('#id' + i + '_' + j).removeClass('cred');
                    $('#id' + i + '_' + j).addClass('cgreen');
		    click[i][j] = 3;
                } else if ((((disp[i] >> j) & 0x1) === 1)&&(((disp[i+8] >> j) & 0x1)===1)) {
		    //$('#id' + i + '_' + j).removeClass('cred');
		    //$('#id' + i + '_' + j).removeClass('cgreen');
                    $('#id' + i + '_' + j).addClass('corange');
		    click[i/2][j] = 2;
		} else if ((((disp[i] >> j) | 0x0) === 0)&&(((disp[i+8] >> j) & 0x1)===1)) {
		    //$('#id' + i + '_' + j).removeClass('corange');
		    //$('#id' + i + '_' + j).removeClass('cgreen');
                    $('#id' + i + '_' + j).addClass('cred');
		    click[i][j] = 1;
		} else
		{
                   // $('#id' + i + '_' + j).removeClass('cred');
		   // $('#id' + i + '_' + j).removeClass('corange');
		   // $('#id' + i + '_' + j).removeClass('cgreen');
		    //$('#id' + i + '_' + j).addClass('cnone');
		    click[i][j] = 0;
                }
            }
        }
    }

    function status_update(txt){
	$('#status').html(txt);
    }

    function updateFromLED(){
      socket.emit("matrix", i2cNum);    
    }

connect();

$(function () {
    // setup control widget
    $("#i2cNum").val(i2cNum).change(function () {
        i2cNum = $(this).val();
    });
});
