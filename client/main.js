let gl;
let glCanvas;
let programInfo;
let previousTime = 0.0;

let entities = [];
let player;
let camera;

window.addEventListener("load", init, false);

function init() {
    glCanvas = document.getElementById("glcanvas");
    glCanvas.width = document.body.clientWidth;
    glCanvas.height = document.body.clientHeight;

    initGL();
    initInputs();
    initNetwork();

    player = new Player([0, 0]);
    entities[-1] = player;
    camera = new DebugCamera()
    entities[-2] = camera;

    window.requestAnimationFrame(function (currentTime) {
        loop(currentTime);
    });

}

function loop(currentT) {
    let d = ((currentT - previousTime) / 1000.0)
    previousTime = currentT;
    sync();
    update(d);
    draw();
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

    for (e in entities)
        entities[e].draw();
}

function initGL() {
    gl = glCanvas.getContext("webgl");
    if (!gl) {
        alert('Unable to initialize WebGL. Your browser or machine may not support it.');
        return;
    }

    gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    gl.enable(gl.BLEND);

    const shaderSet = [
    {
        type: gl.VERTEX_SHADER,
        id: "vertex-shader"
    },
    {
        type: gl.FRAGMENT_SHADER,
        id: "fragment-shader"
    }
    ];

    shaderProgram = buildShaderProgram(gl, shaderSet);

    programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
            textureCoord: gl.getAttribLocation(shaderProgram, 'aTextureCoord'),
        },
        uniformLocations: {
            scalingFactor: gl.getUniformLocation(shaderProgram, 'uScalingFactor'),
            rotationVector: gl.getUniformLocation(shaderProgram, 'uRotationVector'),
            translationVector: gl.getUniformLocation(shaderProgram, 'uTranslationVector'),
            camVector: gl.getUniformLocation(shaderProgram, 'uCamVector'),
            uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
        },
    };
}
