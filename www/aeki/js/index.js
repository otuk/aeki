
var b = [0, 0];  // state of the buttons
var cids =["#buttonThree", "#buttonFour"];  // element id of the buttons
var nos =["1", "2"];  //iot device relay numbers matching to each button

function getAndSetButtonState(){
    //console.log("calling ")
    $.get("cgi/iot.fcgi/status?serviceName=LR01", function(data) {
	})
	.done( function(data) {
	    //console.log("done")
	    if (data["relay1"] == 1)
		b[0] = 1;
	    else
		b[0] = 0;
	    if (data["relay2"] == 1)
		b[1] = 1;
	    else
		b[1] = 0;
	    for (var i=0; i<b.length; i++){
		if (b[i]==1)
		    $(cids[i]).prop("checked", true);
		else
		    $(cids[i]).prop("checked", false);
	    }
	})
        .fail( function(data){
	    //set some messages red
	    console.log("status call failed - cannot get initial status - red shows failure for switches");	    
	    console.log(data);
	    for (var i=0; i<b.length; i++){
		$(cids[i]).next().css("color", "#FF0000");
		if (b[i]==1)
		    $(cids[i]).prop("checked", true);
		else
		    $(cids[i]).prop("checked", false);
	    }
	}
	);
   
}


function setIOTstate(rnum){
    //console.log("reset color status to -trying- not white not red");
    $(cids[rnum]).next().css("color", "#AAAAAA");
    $.get("cgi/iot.fcgi/control?serviceName=LR01&no="+nos[rnum], function(data) {
	})
	.done( function(data) {
	    //console.log("done")
	    if (data["relay1"] == 1)
		b[0] = 1;
	    else
		b[0] = 0;
	    if (data["relay2"] == 1)
		b[1] = 1;
	    else
		b[1] = 0;
	    for (var i=0; i<b.length; i++){
		//success set color to white
		$(cids[i]).next().css("color", "#FFFFFF");
		if (b[i]==1)
		    $(cids[i]).prop("checked", true);
		else
		    $(cids[i]).prop("checked", false);
	    }
	})
        .fail( function(data){
	    console.log("set call failed");
	    $(cids[rnum]).prop("checked", b[rnum] ? true : false);
	    $(cids[rnum]).next().css("color", "#FF0000");
	}
	);
   
}


var makeSwitchHandler = function(num){
    return function(){
	setIOTstate(num);
    }
};


getAndSetButtonState();

for (var i=0; i<b.length; i++){
    $(cids[i]).click(makeSwitchHandler(i));
}; 

