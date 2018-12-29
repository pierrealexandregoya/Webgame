let gl;
let glCanvas;
let programInfo;
let previousTime = 0.0;

let entities = [];
let player;
let camera;

let ready = false;

window.addEventListener("load", _init, false);

if (typeof (debug) === 'undefined')
    debug = false;

// To be removed, server should handle instant reconnection
function _init() {
    setTimeout(init, 2000);
}

function init() {
    glCanvas = document.getElementById("glcanvas");
    glCanvas.width = document.body.clientWidth;
    glCanvas.height = document.body.clientHeight;

    initGL();
    initInputs();
    initNetwork();

    window.requestAnimationFrame(function (currentTime) {
        loop(currentTime);
    });

}

function loop(currentT) {
    let d = ((currentT - previousTime) / 1000.0)
    previousTime = currentT;
    if (ready) {
        sync(d);
        update(d);
        reset_inputs();
        draw();
    }
    window.requestAnimationFrame(function (currentTime) {
        loop(currentTime);
    });
}

function update(d) {
    for (e in entities)
        entities[e].update(d);
}

function draw() {
    gl.viewport(0, 0, glCanvas.width, glCanvas.height);
    gl.clearColor(1.0, 1.0, 1.0, 1.0); // white
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.useProgram(programInfo.program);
    gl.uniform2fv(programInfo.uniformLocations.camVector, camera.pos);

    for (i in entities) {
        if (entities[i].type.search("object") !== -1) {
            entities[i].draw();
        }
    }
    for (i in entities) {
        if (entities[i].type.search("object") === -1)
            entities[i].draw();
    }
}
