let currentlyPressedKeys = {};
let mouseDown = false;
let mousePos = [];

function initInputs() {
    document.onkeydown = handleKeyDown;
    document.onkeyup = handleKeyUp;
    glCanvas.onmousemove = onMouseMove;
    glCanvas.onmousedown = onMouseDown;
    glCanvas.onmouseup = onMouseUp;
}

function onMouseDown(event) {
    mouseDown = true;
}

function onMouseUp(event) {
    mouseDown = false;
}

function onMouseMove(e) {
    mousePos = [e.clientX, e.clientY];
}

function handleKeyDown(event) {
    currentlyPressedKeys[event.key] = true;
}

function handleKeyUp(event) {
    currentlyPressedKeys[event.key] = false;
}
