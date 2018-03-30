// Aspect ratio and coordinate system
// details

let aspectRatio;
let currentRotation = [0, 1];
let currentScale = [1.0, 1.0];

// Vertex information

let vertexArray;
let vertexBuffer;
let vertexNumComponents;
let vertexCount;

// Rendering data shared with the
// scalers.

// let uScalingFactor;
// let uGlobalColor;
// let uRotationVector;
// let aVertexPosition;

// Animation timing

let previousTime = 0.0;
let degreesPerSecond = 90.0;

window.addEventListener("load", main, false);

function main() {
    const glCanvas = document.getElementById("glcanvas");
    glCanvas.width = document.body.clientWidth; //document.width is obsolete
    glCanvas.height = document.body.clientHeight; //document.height is obsolete
    
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
	    uSampler: gl.getUniformLocation(shaderProgram, 'uSampler'),
	},
    };

    aspectRatio = glCanvas.width/glCanvas.height;
    currentRotation = [0, 1];
    currentScale = [1.0, aspectRatio];

    // vertexArray = new Float32Array([
    // 	    -0.5, 0.5, 0.5, 0.5, 0.5, -0.5,
    // 	    -0.5, 0.5, 0.5, -0.5, -0.5, -0.5
    // ]);

    // vertexBuffer = gl.createBuffer();
    // gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    // gl.bufferData(gl.ARRAY_BUFFER, vertexArray, gl.STATIC_DRAW);
    
    // vertexNumComponents = 2;
    // vertexCount = vertexArray.length/vertexNumComponents;

    currentAngle = 0.0;
    rotationRate = 6;

    const buffers = initBuffers(gl);
    const texture = loadTexture(gl, 'knight256.png');
    
    animateScene(gl, glCanvas, buffers, texture, programInfo);
}

function animateScene(gl, glCanvas, buffers, texture, programInfo) {
    gl.viewport(0, 0, glCanvas.width, glCanvas.height);
    gl.clearColor(0.8, 0.9, 1.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    let radians = currentAngle * Math.PI / 180.0;
    currentRotation[0] = Math.sin(radians);
    currentRotation[1] = Math.cos(radians);

    gl.useProgram(programInfo.program);

    // uScalingFactor =
    // 	gl.getUniformLocation(shaderProgram, "uScalingFactor");
    // uGlobalColor =
    // 	gl.getUniformLocation(shaderProgram, "uGlobalColor");
    // uRotationVector =
    // 	gl.getUniformLocation(shaderProgram, "uRotationVector");

    gl.uniform2fv(programInfo.uniformLocations.scalingFactor, currentScale);
    gl.uniform2fv(programInfo.uniformLocations.rotationVector, currentRotation);
    // gl.uniform4fv(uGlobalColor, [0.1, 0.7, 0.2, 1.0]);

    // gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);

    // aVertexPosition =
    // 	gl.getAttribLocation(shaderProgram, "aVertexPosition");

    
    // gl.enableVertexAttribArray(aVertexPosition);
    // gl.vertexAttribPointer(aVertexPosition, vertexNumComponents,
    // 			   gl.FLOAT, false, 0, 0);

    // gl.drawArrays(gl.TRIANGLES, 0, vertexCount);

    // Tell WebGL how to pull out the positions from the position
    // buffer into the vertexPosition attribute
    {
	const numComponents = 2;
	const type = gl.FLOAT;
	const normalize = false;
	const stride = 0;
	const offset = 0;
	gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
	gl.vertexAttribPointer(
	    programInfo.attribLocations.vertexPosition,
	    numComponents,
	    type,
	    normalize,
	    stride,
	    offset);
	gl.enableVertexAttribArray(
	    programInfo.attribLocations.vertexPosition);
    }

    // Tell WebGL how to pull out the texture coordinates from
    // the texture coordinate buffer into the textureCoord attribute.
    {
	const numComponents = 2;
	const type = gl.FLOAT;
	const normalize = false;
	const stride = 0;
	const offset = 0;
	gl.bindBuffer(gl.ARRAY_BUFFER, buffers.textureCoord);
	gl.vertexAttribPointer(
	    programInfo.attribLocations.textureCoord,
	    numComponents,
	    type,
	    normalize,
	    stride,
	    offset);
	gl.enableVertexAttribArray(
	    programInfo.attribLocations.textureCoord);
    }

    // Tell WebGL which indices to use to index the vertices
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers.indices);

        // Specify the texture to map onto the faces.

    // Tell WebGL we want to affect texture unit 0
    gl.activeTexture(gl.TEXTURE0);

    // Bind the texture to texture unit 0
    gl.bindTexture(gl.TEXTURE_2D, texture);

    // Tell the shader we bound the texture to texture unit 0
    gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

    {
	const vertexCount = 6;
	const type = gl.UNSIGNED_SHORT;
	const offset = 0;
	gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
    }
    
    window.requestAnimationFrame(function(currentTime) {
	let deltaAngle = ((currentTime - previousTime) / 1000.0)
	    * degreesPerSecond;

	currentAngle = (currentAngle + deltaAngle) % 360;

	previousTime = currentTime;
	animateScene(gl, glCanvas, buffers, texture, programInfo);
    });
}

function initBuffers(gl) {
    // Create a buffer for the cube's vertex positions.

    const positionBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, positionBuffer);
    const positions = [
	-.1, -.1,
	.1, -.1,
	.1,  .1,
	-.1,  .1,
    ];
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(positions), gl.STATIC_DRAW);

    const textureCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, textureCoordBuffer);
    const textureCoordinates = [
	0.0,  0.0,
	1.0,  0.0,
	1.0,  1.0,
	0.0,  1.0,
    ];
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(textureCoordinates),
		  gl.STATIC_DRAW);

    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer);
    const indices = [
	0,  1,  2,      0,  2,  3,    // front
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

    shaderInfo.forEach(function(desc) {
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
    image.onload = function() {
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

// function drawScene(gl, programInfo, buffers, texture, deltaTime) {
//     gl.clearColor(0.0, 0.0, 0.0, 1.0);  // Clear to black, fully opaque
//     gl.clearDepth(1.0);                 // Clear everything
//     gl.enable(gl.DEPTH_TEST);           // Enable depth testing
//     gl.depthFunc(gl.LEQUAL);            // Near things obscure far things

//     // Clear the canvas before we start drawing on it.

//     gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

//     // Create a perspective matrix, a special matrix that is
//     // used to simulate the distortion of perspective in a camera.
//     // Our field of view is 45 degrees, with a width/height
//     // ratio that matches the display size of the canvas
//     // and we only want to see objects between 0.1 units
//     // and 100 units away from the camera.

//     const fieldOfView = 45 * Math.PI / 180;   // in radians
//     const aspect = gl.canvas.clientWidth / gl.canvas.clientHeight;
//     const zNear = 0.1;
//     const zFar = 100.0;
//     const projectionMatrix = mat4.create();

//     // note: glmatrix.js always has the first argument
//     // as the destination to receive the result.
//     mat4.perspective(projectionMatrix,
// 		     fieldOfView,
// 		     aspect,
// 		     zNear,
// 		     zFar);

//     // Set the drawing position to the "identity" point, which is
//     // the center of the scene.
//     const modelViewMatrix = mat4.create();

//     // Now move the drawing position a bit to where we want to
//     // start drawing the square.

//     mat4.translate(modelViewMatrix,     // destination matrix
// 		   modelViewMatrix,     // matrix to translate
// 		   [-0.0, 0.0, -6.0]);  // amount to translate
//     mat4.rotate(modelViewMatrix,  // destination matrix
// 		modelViewMatrix,  // matrix to rotate
// 		cubeRotation,     // amount to rotate in radians
// 		[0, 0, 1]);       // axis to rotate around (Z)
//     mat4.rotate(modelViewMatrix,  // destination matrix
// 		modelViewMatrix,  // matrix to rotate
// 		cubeRotation * .7,// amount to rotate in radians
// 		[0, 1, 0]);       // axis to rotate around (X)

//     // Tell WebGL how to pull out the positions from the position
//     // buffer into the vertexPosition attribute
//     {
// 	const numComponents = 3;
// 	const type = gl.FLOAT;
// 	const normalize = false;
// 	const stride = 0;
// 	const offset = 0;
// 	gl.bindBuffer(gl.ARRAY_BUFFER, buffers.position);
// 	gl.vertexAttribPointer(
// 	    programInfo.attribLocations.vertexPosition,
// 	    numComponents,
// 	    type,
// 	    normalize,
// 	    stride,
// 	    offset);
// 	gl.enableVertexAttribArray(
// 	    programInfo.attribLocations.vertexPosition);
//     }

//     // Tell WebGL how to pull out the texture coordinates from
//     // the texture coordinate buffer into the textureCoord attribute.
//     {
// 	const numComponents = 2;
// 	const type = gl.FLOAT;
// 	const normalize = false;
// 	const stride = 0;
// 	const offset = 0;
// 	gl.bindBuffer(gl.ARRAY_BUFFER, buffers.textureCoord);
// 	gl.vertexAttribPointer(
// 	    programInfo.attribLocations.textureCoord,
// 	    numComponents,
// 	    type,
// 	    normalize,
// 	    stride,
// 	    offset);
// 	gl.enableVertexAttribArray(
// 	    programInfo.attribLocations.textureCoord);
//     }

//     // Tell WebGL which indices to use to index the vertices
//     gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, buffers.indices);

//     // Tell WebGL to use our program when drawing

//     gl.useProgram(programInfo.program);

//     // Set the shader uniforms

//     gl.uniformMatrix4fv(
// 	programInfo.uniformLocations.projectionMatrix,
// 	false,
// 	projectionMatrix);
//     gl.uniformMatrix4fv(
// 	programInfo.uniformLocations.modelViewMatrix,
// 	false,
// 	modelViewMatrix);

//     // Specify the texture to map onto the faces.

//     // Tell WebGL we want to affect texture unit 0
//     gl.activeTexture(gl.TEXTURE0);

//     // Bind the texture to texture unit 0
//     gl.bindTexture(gl.TEXTURE_2D, texture);

//     // Tell the shader we bound the texture to texture unit 0
//     gl.uniform1i(programInfo.uniformLocations.uSampler, 0);

//     {
// 	const vertexCount = 36;
// 	const type = gl.UNSIGNED_SHORT;
// 	const offset = 0;
// 	gl.drawElements(gl.TRIANGLES, vertexCount, type, offset);
//     }

//     // Update the rotation for the next draw

//     cubeRotation += deltaTime;
// }
