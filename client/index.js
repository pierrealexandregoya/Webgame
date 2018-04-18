let gl;
let glCanvas;
let programInfo;
let previousTime = 0.0;

let entities = [];
let player;
let camera;

let currentlyPressedKeys = {};
let mouseDown = false;
let mousePos = [];

window.addEventListener("load", main, false);

class Entity {
    constructor(pos, texUrl) {
        this.pos = pos;
        this.texUrl = texUrl;
        this.vel = [0, 0];
        this.rot = [0, 1];
        this.angle = 0;
        this.scale = [1, 1];
        this.texGl = loadTexture(gl, this.texUrl);
        this.buffers = initBuffers(gl);
    }

    update(d) {
    }

    draw() {
        this.scale[0] = glCanvas.height / glCanvas.width;

        gl.uniform2fv(programInfo.uniformLocations.scalingFactor, this.scale);
        gl.uniform2fv(programInfo.uniformLocations.rotationVector, this.rot);
        gl.uniform2fv(programInfo.uniformLocations.translationVector, this.pos);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers.position);
        gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
        gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

        gl.bindBuffer(gl.ARRAY_BUFFER, this.buffers.textureCoord);
        gl.vertexAttribPointer(programInfo.attribLocations.textureCoord, 2, gl.FLOAT, false, 0, 0);
        gl.enableVertexAttribArray(programInfo.attribLocations.textureCoord);

        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.buffers.indices);

        gl.activeTexture(gl.TEXTURE0);

        gl.bindTexture(gl.TEXTURE_2D, this.texGl);

        gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

        gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
    }
}

class Player extends Entity {
    constructor(pos) {
        super(pos, "knight256.png");
        this.move = false;
    }

    update(d) {
        if (mouseDown) {
            let cursorInWorld = screenToWorld(mousePos);
            if (getNorm([cursorInWorld[0] - this.pos[0], cursorInWorld[1] - this.pos[1]]) > 0.01) {
                this.targetPos = [cursorInWorld[0], cursorInWorld[1]];
                this.vel = normalize([this.targetPos[0] - this.pos[0], this.targetPos[1] - this.pos[1]]);
                this.rot[1] = dotProduct(this.vel, [0, 1]) / (getNorm(this.vel) * getNorm([0, 1]));
                this.rot[0] = Math.sqrt(1 - this.rot[1] * this.rot[1]);
                if (this.vel[0] < 0)
                    this.rot[0] *= -1;
                this.move = true;
            }
        }

        if (this.move && Math.abs(this.targetPos[0] - this.pos[0]) < 0.011
            && Math.abs(this.targetPos[1] - this.pos[1]) < 0.1) {
            this.vel = [0, 0]
            this.move = false;
        }

        this.pos = [this.pos[0] + this.vel[0] * d, this.pos[1] + this.vel[1] * d];
    }
}

class DebugCamera extends Entity {
    constructor() {
        super([0, 0], "");
    }

    update(d) {
        this.vel = [0, 0];
        let step = 1;
        if (currentlyPressedKeys['Control'])
            step *= 5;
        if (currentlyPressedKeys['ArrowUp'])
            this.vel[1] += 1;
        if (currentlyPressedKeys['ArrowDown'])
            this.vel[1] -= 1;
        if (currentlyPressedKeys['ArrowLeft'])
            this.vel[0] -= 1;
        if (currentlyPressedKeys['ArrowRight'])
            this.vel[0] += 1;


        this.vel = normalize(this.vel);
        this.vel[0] *= step;
        this.vel[1] *= step;
        this.pos[0] += this.vel[0] * d;
        this.pos[1] += this.vel[1] * d;
    }

    draw() {
    }
}

function main() {
    var exampleSocket = new WebSocket("ws://" + window.location.host + ":2000");

    exampleSocket.onopen = function (event) {
        exampleSocket.send("Here's some text that the server is urgently awaiting!");
    };

    exampleSocket.onmessage = function (event) {
        order = JSON.parse(event.data);
        for (i in order.data) {

            id = order.data[i].id;
            newPos = [order.data[i].pos.x, order.data[i].pos.y];
            if (entities.hasOwnProperty(id)) {
                //console.log(id.toString() + " is in game with pos " + entities[id].pos.toString() + " and newPos " + newPos.toString());
                entities[id].pos = newPos;
            }
            else {
                console.log(id.toString() + " is not in game");
                if (order.data[i].type === "enemy1")
                    entities[id] = new Entity(newPos, "knight64.png");
                else if (order.data[i].type === "object1")
                    entities[id] = new Entity(newPos, "cross16.png");
                else {
                    console.log("Unknown type: " + order.data[i].type)
                    entities[id] = new Entity(newPos, "");
                }

            }
        }
    }

    glCanvas = document.getElementById("glcanvas");
    glCanvas.width = document.body.clientWidth;
    glCanvas.height = document.body.clientHeight;

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

    document.onkeydown = handleKeyDown;
    document.onkeyup = handleKeyUp;
    glCanvas.onmousemove = onMouseMove;
    glCanvas.onmousedown = onMouseDown;
    glCanvas.onmouseup = onMouseUp;

    //let s = 10;
    //for (i = 0; i < s; ++i)
    //    for (j = 0; j < s; ++j)
    //        entities.push(new Entity([i - s / 2, j - s / 2], "cross16.png"));

    entities.push(new Entity([0.9, 0.9], "knight64.png"));

    player = new Player([-0.2, 0]);
    entities.push(player);
    camera = new DebugCamera()
    entities.push(camera);


    window.requestAnimationFrame(function (currentTime) {
        loop(currentTime);
    });

}

function loop(currentT) {
    let d = ((currentT - previousTime) / 1000.0)
    previousTime = currentT;
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
    // gl.clearColor(0.0, .0, .0, 1.0); // black
    gl.clearColor(1.0, 1.0, 1.0, 1.0); // white
    gl.clear(gl.COLOR_BUFFER_BIT);

    gl.useProgram(programInfo.program);
    gl.uniform2fv(programInfo.uniformLocations.camVector, camera.pos);

    for (e in entities)
        entities[e].draw();
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

function screenToWorld(screenPos) {
    let w = (glCanvas.width / glCanvas.height);
    let x = (screenPos[0] / glCanvas.width);
    let y = (screenPos[1] / glCanvas.height);
    x = -w + camera.pos[0] + (x * (w * 2));
    y = -1 + camera.pos[1] + (2 - 2 * y);
    return [x, y];
}

function dotProduct(array1, array2) {
    let r = 0;
    for (i in array1)
        r += array1[i] * array2[i];
    return r;
}

function getNorm(array) {
    let tot = 0;

    for (i in array) {
        tot += array[i] * array[i];
    }
    return Math.sqrt(tot);
}

function normalize(array) {
    let norm = getNorm(array);
    let r = array
    if (norm !== 0) {
        r[0] /= norm;
        r[1] /= norm;
    }
    return r;
}

function initBuffers(gl) {
    // Create a buffer for the cube's vertex positions.

    const positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    const positions = [
        -.1, -.1,
        .1, -.1,
        .1, .1,
        -.1, .1,
    ];
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

    const textureCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);
    const textureCoordinates = [
    0.0, .0,
    1.0, .0,
    1.0, 1.0,
    0.0, 1.0,
    ];
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinates),
          gl.STATIC_DRAW);

    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    const indices = [
    0, 1, 2, 0, 2, 3,    // front
    ];
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER,
          new Uint16Array(indices), gl.STATIC_DRAW);

    return {
        position: positionBuffer,
        textureCoord: textureCoordBuffer,
        indices: indexBuffer,
    };
}

function buildShaderProgram(gl, shaderInfo) {
    let program = gl.createProgram();

    shaderInfo.forEach(function (desc) {
        let shader = compileShader(gl, desc.id, desc.type);

        if (shader) {
            gl.attachShader(program, shader);
        }
    });

    gl.linkProgram(program)

    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        console.log("Error linking shader program:");
        console.log(gl.getProgramInfoLog(program));
    }

    return program;
}

function compileShader(gl, id, type) {
    let code = document.getElementById(id).firstChild.nodeValue;
    let shader = gl.createShader(type);

    gl.shaderSource(shader, code);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        console.log(`Error compiling ${type === gl.VERTEX_SHADER ? "vertex" : "fragment"} shader:`);
        console.log(gl.getShaderInfoLog(shader));
    }
    return shader;
}

function loadTexture(gl, url) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);

    // Because images have to be download over the internet
    // they might take a moment until they are ready.
    // Until then put a single pixel in the texture so we can
    // use it immediately. When the image has finished downloading
    // we'll update the texture with the contents of the image.
    const level = 0;
    const internalFormat = gl.RGBA;
    const width = 1;
    const height = 1;
    const border = 0;
    const srcFormat = gl.RGBA;
    const srcType = gl.UNSIGNED_BYTE;
    const pixel = new Uint8Array([0, 0, 255, 255]);  // opaque blue
    gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
          width, height, border, srcFormat, srcType,
          pixel);

    const image = new Image();
    image.onload = function () {
        gl.bindTexture(gl.TEXTURE_2D, texture);
        gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
                  srcFormat, srcType, image);

        // WebGL1 has different requirements for power of 2 images
        // vs non power of 2 images so check if the image is a
        // power of 2 in both dimensions.
        if (isPowerOf2(image.width) && isPowerOf2(image.height)) {
            // Yes, it's a power of 2. Generate mips.
            gl.generateMipmap(gl.TEXTURE_2D);
        } else {
            // No, it's not a power of 2. Turn of mips and set
            // wrapping to clamp to edge
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
        }
    };
    image.src = url;

    return texture;
}

function isPowerOf2(value) {
    return (value & (value - 1)) === 0;
}
