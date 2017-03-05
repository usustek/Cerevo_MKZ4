var lastVal = {
    method: "",
    axel: 0,
    steer: 0
};

var initVal = {
    init: false,
    axel: 0,
    steer:0
};

//var host = "192.168.10.8";
var host = location.host;
var ws = undefined;

function checkPad()
{
    var pads = navigator.getGamepads();
    if(pads){
        for(var i = 0; i < pads.length; i ++)
        {
            var pad = pads[i];
            if(pad)
            {
                if(pad.id.lastIndexOf("Unknown") >= 0) {
                    continue;
                }

                var start = pad.buttons[9]; // start btton
                if(start && start.pressed && ws) {
                    sendPadData("init");
                    continue;
                }

                var chg = updatePad(pad);
                if(chg && ws) {
                    sendPadData("control");
                }
            }
        }
    }
}

/**
 * @param  {Gamepad} pad
 * @returns {boolean}
 */
function updatePad(pad)
{
    var ax1 = pad.axes[1]; // axel
    var ax2 = pad.axes[2]; // steering
    var chg = false;

    if(lastVal.axel != ax1) {
        lastVal.axel = ax1;
        chg |= true;
    }

    if(lastVal.steer != ax2) {
        lastVal.steer = ax2;
        chg |= true;
    }

    if(chg) {
        if(!initVal.init){
            initVal.axel  = lastVal.axel;
            initVal.steer = lastVal.steer;
            initVal.init  = true;
        }
        console.log(lastVal.axel + " / " + lastVal.steer);
    }

    return chg;
}

/**
 * @param {string} method
 */
function sendPadData(method){
    if(ws){
        lastVal.method = method;
        ws.send(JSON.stringify(lastVal));
    }
}

function setIntervalCheck() {
    window.requestAnimationFrame(setIntervalCheck);
    checkPad();
};

$("#con").click(function(){
    var btn = $("#con");

    if(ws){
        ws.close();
    }
    else{
        var sock = new WebSocket("ws://" + host + "/ws");
        sock.onopen = function(event){
            ws = this; //WebSocket
            btn.text("Disconnect");
            window.requestAnimationFrame(setIntervalCheck);
            sendPadData("init");
        };
        sock.onclose = function(event){
            ws = undefined;
            btn.text("Connect");
            window.cancelAnimationFrame(setIntervalCheck);
        };

    }
});

window.onload = function(){
    checkPad();
};
