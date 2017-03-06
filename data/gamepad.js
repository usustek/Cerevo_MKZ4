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

var PadDef = {

};

//var host = "192.168.10.8";
var host = location.host;
var ws = undefined;

function checkPad()
{
    var pads = navigator.getGamepads();
    var def = PadDef;
    if(def && pads.length > def.index && pads[def.index]){
        var pad = pads[def.index];
        if(pad)
        {
            var start = pad.buttons[def.startButton]; // start btton
            if(start && start.pressed) {
                toggleConnect();
            }

            var chg = updatePad(pad, def);
            if(/*chg &&*/ ws) {
                sendPadData("control");
            }
        }
    }
}

function initPad()
{
    var pads = navigator.getGamepads();
    var def = PadDef;
    if(def && pads.length > def.index && pads[def.index]){
        var pad = pads[def.index];
        if(pad)
        {
            var chg = updatePad(pad, def);
            if(chg) {
                initVal.axel  = lastVal.axel;
                initVal.steer = lastVal.steer;
                initVal.init  = true;
                console.log(lastVal.axel + " / " + lastVal.steer);
            }
        }
    }
}

/**
 * @param  {Gamepad} pad
 * @param  {PadDef} def
 * @returns {boolean}
 */
function updatePad(pad, def)
{
    var ax1 = pad.axes[def.axelAxis];  // axel
    var ax2 = pad.axes[def.steerAxis]; // steering
    var chg = false;

    if(lastVal.axel != ax1) {
        lastVal.axel = ax1;
        chg |= true;
    }

    if(lastVal.steer != ax2) {
        lastVal.steer = ax2;
        chg |= true;
    }

    return chg;
}

function map(val, in_min, in_max, out_min, out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
/**
 * @param {string} method
 */
function sendPadData(method){
    if(ws){
        var snd = {
            method: method,
            axel: -lastVal.axel,
            steer: lastVal.steer
        };

        ws.send(JSON.stringify(snd));
    }
}

function setIntervalCheck() {
    window.requestAnimationFrame(setIntervalCheck);
    checkPad();
};

function toggleConnect(){
    var btn = $("#con");

    if(ws){
        ws.close();
    }
    else{
        var sock = new WebSocket("ws://" + host + "/ws");
        sock.onopen = function(event){
            ws = this; //WebSocket
            btn.text("Disconnect");
            sendPadData("init");
        };
        sock.onclose = function(event){
            ws = undefined;
            btn.text("Connect");
        };

    }
};

var GamePadDefs = {
    "Logicool Dual Action (STANDARD GAMEPAD Vendor: 046d Product: c216)" : {
        axelAxis: 1,
        steerAxis: 2,
        startButton: 9
    }
};

window.onload = function(){
    $("#con").click(toggleConnect);
};

window.addEventListener("gamepadconnected",  function(e){
    console.log("Gamepad connected at index %d: %s. %d buttons, %d axes.",
        e.gamepad.index, e.gamepad.id,
        e.gamepad.buttons.length, e.gamepad.axes.length);

    var def = GamePadDefs[e.gamepad.id];
    if(def){
        def.id    = e.gamepad.id;
        def.index = e.gamepad.index;
        PadDef = def;
    }
    else{
        var gp = e.gamepad;
        var def = {
            id: gp.id,
            index: gp.index
        };

        if(gp.axes.length == 2){
            def.axelAxis    = 0;
            def.steerAxis   = 1;
            def.startButton = 0;
        }
        else if(gp.axes.length == 4){
            def.axelAxis    = 1;
            def.steerAxis   = 2;
            def.startButton = 0;
        }

        PadDef = def;
    }

    initPad();
    window.requestAnimationFrame(setIntervalCheck);
}, false);

window.addEventListener("gamepaddisconnected", function(e) {
    console.log("Gamepad disconnected from index %d: %s",
        e.gamepad.index, e.gamepad.id);
    PadDef = null;
    window.cancelAnimationFrame(setIntervalCheck);
}, false);
