let currentlyPressedKeys = {};

let leftMouseDown = false;
let leftMouseClicked = false;

let rightMouseDown = false;
let rightMouseClicked = false;

let mousePos = [0, 0];

function initInputs() {
    document.onkeydown = handleKeyDown;
    document.onkeyup = handleKeyUp;
    glCanvas.onmousedown = handleMouseDown;
    glCanvas.onmouseup = handleMouseUp;
    glCanvas.onmousemove = handleMouseMove;
    glCanvas.oncontextmenu = inhibitContextMenu;
}

function reset_inputs() {
    leftMouseClicked = false;
    rightMouseClicked = false;
}

function handleKeyDown(event) {
    currentlyPressedKeys[event.key] = true;
}

function handleKeyUp(event) {
    currentlyPressedKeys[event.key] = false;
}

function handleMouseDown(event) {
    switch (event.which) {
        case 1:
            leftMouseDown = true;
            leftMouseClicked = true;
            break;
        case 2:
            break;
        case 3:
            rightMouseDown = true;
            rightMouseClicked = true;
            break;
    }
}

function handleMouseUp(event) {
    switch (event.which) {
        case 1:
            leftMouseDown = false;
            break;
        case 2:
            break;
        case 3:
            rightMouseDown = false;
            break;
    }
}

function handleMouseMove(e) {
    mousePos = [e.clientX, e.clientY];
}

function inhibitContextMenu(event) {
    event.preventDefault();
    return false;
}
