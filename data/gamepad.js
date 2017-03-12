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

// var host = "192.168.10.10";
var host = location.host;

/**
 * @param {WebSocket} ws
 */
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

            if(!updateTrim(pad, def)){
                var chg = updatePad(pad, def);
                if(/*chg &&*/ ws) {
                    sendPadData("control");
                }
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

function updateTrim(pad, def){
    var chg = false;
    var snd = {
        trimAxel: 0,
        trimSteer: 0
    };

    if(def.trimLeft || def.trimRight){
        snd.trimSteer -= pad.buttons[def.trimLeft].pressed ? 1 : 0;
        snd.trimSteer += pad.buttons[def.trimRight].pressed ? 1 : 0;
        chg |= snd.trimSteer != 0;
    }

    if(def.trimForward || def.trimBack){
        snd.trimAxel += pad.buttons[def.trimForward].pressed ? 1 : 0;
        snd.trimAxel -= pad.buttons[def.trimBack].pressed ? 1 : 0;
        chg |= snd.trimAxel != 0;
    }

    if(chg){
        snd.method = "trim";
        sendCommand(snd);
    }

    return chg;
}

function sendCommand(obj) {
    if(ws && (ws.readyState == 1) && (ws.bufferedAmount == 0)){
        var dat = JSON.stringify(obj);
        ws.send(dat);
        console.debug(dat);
    }
}

function map(val, in_min, in_max, out_min, out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

var getPast = (function() {
  var lastDate = NaN;
  return function(update) {
    var now = Date.now();
    var past = now - lastDate;
    if(update){
        lastDate = now;
    }
    return past;
  };
})();

/**
 * @param {string} method
 */
function sendPadData(method){
    var past = getPast(false);

    if(ws && (ws.readyState == 1) && (ws.bufferedAmount == 0) && (past > 50)){
        var snd = {
            method: method,
            axel: -lastVal.axel,
            steer: lastVal.steer
        };

        var dt = JSON.stringify(snd);
        ws.send(dt);
        console.debug(dt);
        getPast(true);
    }
}

function setIntervalCheck() {
    window.requestAnimationFrame(setIntervalCheck);
    checkPad();
};

function toggleConnect(){
    var btn = $("#con");

    if(ws && (ws.readyState == 0 || ws.readyState == 2)){
        return;
    }

    if(!ws || ws.readyState == 1){
        var sock = new WebSocket("ws://" + host + "/ws");
        sock.readyState
        sock.onopen = function(event){
            btn.text("Disconnect");
            getPast(true);
        };
        sock.onclose = function(event){
            ws = undefined;
            btn.text("Connect");
        };
        ws = sock; //WebSocket
    }
    else{
        ws.close();
    }
};

var GamePadDefs = {
    "Logicool Dual Action (STANDARD GAMEPAD Vendor: 046d Product: c216)" : {
        axelAxis: 1,
        steerAxis: 2,
        trimForward: 12,
        trimBack: 13,
        trimLeft: 14,
        trimRight: 15,
        startButton: 9
    },
    /* PS4 */
    "Wireless Controller (STANDARD GAMEPAD Vendor: 054c Product: 05c4)" :{
        axelAxis: 1,
        steerAxis: 2,
        startButton: 9
    }

};

/**
 * 
 * @param {Gamepad} pad 
 */
function initPadDef(pad){
    var def = GamePadDefs[pad.id];
    if(def){
        def.id    = pad.id;
        def.index = pad.index;
        PadDef    = def;
    }
    else{
        var gp = pad;
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
            if(gp.buttons.length > 9){
                def.startButton = 9;
            }
        }

        PadDef = def;
    }
}

function searchGamepad() {
    var pads = navigator.getGamepads();

    for(var i = 0; i < pads.length; i ++) {
        var pad = pads[i];

        if(!pad || pad.id.startsWith("Unknown")) {
            continue;
        }

        initPadDef(pad);
        window.requestAnimationFrame(setIntervalCheck);
        break;
    }
}

window.onload = function(){
    $("#con").click(toggleConnect);
    searchGamepad();
};

window.addEventListener("gamepadconnected",  function(e){
    console.log("Gamepad connected at index %d: %s. %d buttons, %d axes.",
        e.gamepad.index, e.gamepad.id,
        e.gamepad.buttons.length, e.gamepad.axes.length);

    initPadDef(e.gamepad);
    initPad();
    window.requestAnimationFrame(setIntervalCheck);
}, false);

window.addEventListener("gamepaddisconnected", function(e) {
    console.log("Gamepad disconnected from index %d: %s",
        e.gamepad.index, e.gamepad.id);
    PadDef = null;
}, false);
