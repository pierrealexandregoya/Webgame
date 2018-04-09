let aspectRatio;
let currentRotation = [0, 1];
let currentScale = [1.0, 1.0];
let currentTranslation = [0, 0];
let currentlyPressedKeys = {};
let previousTime = 0.0;
let degreesPerSecond = 90.0;

window.addEventListener("load", main, false);

function main() {
    const glCanvas = document.getElementById("glcanvas");
    glCanvas.width = document.body.clientWidth;
    glCanvas.height = document.body.clientHeight;

    const gl = glCanvas.getContext("webgl");
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

    const programInfo = {
        program: shaderProgram,
        attribLocations: {
            vertexPosition: gl.getAttribLocation(shaderProgram, 'aVertexPosition'),
            textureCoord: gl.getAttribLocation(shaderProgram, 'aTextureCoord'),
        },
        uniformLocations: {
            scalingFactor: gl.getUniformLocation(shaderProgram, 'uScalingFactor'),
            rotationVector: gl.getUniformLocation(shaderProgram, 'uRotationVector'),
            translationVector: gl.getUniformLocation(shaderProgram, 'uTranslationVector'),
            uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
        },
    };

    aspectRatio = glCanvas.width / glCanvas.height;
    currentRotation = [0, 1];
    currentScale = [1.0, aspectRatio];

    currentAngle = .0;
    rotationRate = 6;

    const buffers = initBuffers(gl);
    const texture = loadTexture(gl, 'knight256.png');

    document.onkeydown = handleKeyDown;
    document.onkeyup = handleKeyUp;

    window.requestAnimationFrame(function (currentTime) {
        loop(currentTime, gl, glCanvas, buffers, texture, programInfo);
    });

}

function loop(currentT, gl, glCanvas, buffers, texture, programInfo) {
    let d = ((currentT - previousTime) / 1000.0)
    previousTime = currentT;
    update(d);
    draw(gl, glCanvas, buffers, texture, programInfo);
    window.requestAnimationFrame(function (currentTime) {
        loop(currentTime, gl, glCanvas, buffers, texture, programInfo);
    });
}

function update(d) {
    if (currentlyPressedKeys[37] == true)
        currentTranslation[0] -= 1 * d;
    if (currentlyPressedKeys[38] == true)
        currentTranslation[1] += 1 * d;
    if (currentlyPressedKeys[39] == true)
        currentTranslation[0] += 1 * d;
    if (currentlyPressedKeys[40] == true)
        currentTranslation[1] -= 1 * d;

    let deltaAngle = d * degreesPerSecond;
    currentAngle = (currentAngle + deltaAngle) % 360;

    if (currentlyPressedKeys[83] == true) {
        currentScale[0] += 1 * d;
        currentScale[1] += 1 * d;
    }
}

function draw(gl, glCanvas, buffers, texture, programInfo) {
    gl.viewport(0, 0, glCanvas.width, glCanvas.height);
    // gl.clearColor(0.0, .0, .0, 1.0); // black
    gl.clearColor(1.0, 1.0, 1.0, 1.0); // white
    gl.clear(gl.COLOR_BUFFER_BIT);

    let radians = currentAngle * Math.PI / 180.0;
    currentRotation[0] = Math.sin(radians);
    currentRotation[1] = Math.cos(radians);

    gl.useProgram(programInfo.program);

    gl.uniform2fv(programInfo.uniformLocations.scalingFactor, currentScale);
    gl.uniform2fv(programInfo.uniformLocations.rotationVector, currentRotation);
    gl.uniform2fv(programInfo.uniformLocations.translationVector, currentTranslation);

    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
    gl.vertexAttribPointer(programInfo.attribLocations.vertexPosition, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(programInfo.attribLocations.vertexPosition);

    gl.bindBuffer(gl.ARRAY_BUFFER, buffers.textureCoord);
    gl.vertexAttribPointer(programInfo.attribLocations.textureCoord, 2, gl.FLOAT, false, 0, 0);
    gl.enableVertexAttribArray(programInfo.attribLocations.textureCoord);

    // Tell WebGL which indices to use to index the vertices
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers.indices);

    // Specify the texture to map onto the faces.

    // Tell WebGL we want to affect texture unit 0
    gl.activeTexture(gl.TEXTURE0);

    // Bind the texture to texture unit 0
    gl.bindTexture(gl.TEXTURE_2D, texture);

    // Tell the shader we bound the texture to texture unit 0
    gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

    gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
}

function handleKeyDown(event) {
    currentlyPressedKeys[event.keyCode] = true;
}

function handleKeyUp(event) {
    currentlyPressedKeys[event.keyCode] = false;
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

//
// Initialize a texture and load an image.
// When the image finished loading copy it into the texture.
//
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
    return (value & (value - 1)) == 0;
}
