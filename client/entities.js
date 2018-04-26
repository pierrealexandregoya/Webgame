class Entity {
    constructor(pos, texUrl) {
        this.pos = pos;
        this.targetPos = pos;
        this.texUrl = texUrl;
        this.vel = [0, 0];
        this.rot = [0, 1];
        this.angle = 0;
        this.scale = [1, 1];
        this.texGl = loadTexture(gl, this.texUrl);
        this.buffers = initBuffers(gl);
    }

    update(d) {
        // testing conflicts with server for pos update
        //if (getNorm(this.vel) > Number.epsilon)


        if (Math.abs(getNorm(this.vel)) > Number.EPSILON) {
            this.rot[1] = dotProduct(this.vel, [0, 1]) / (getNorm(this.vel) * getNorm([0, 1]));
            this.rot[0] = Math.sqrt(1 - this.rot[1] * this.rot[1]);
            if (this.vel[0] < 0)
                this.rot[0] *= -1;
        }
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
        this.nbMove = 0;
    }

    stop(nbMove) {
        if (nbMove != this.nbMove)
            return;
        console.log("stop");
        this.vel = [0, 0]
        this.move = false;
        syncNeeded = true;
    }

    update(d) {
        if (mouseDown) {
            let cursorInWorld = screenToWorld(mousePos);
            // If new target if far enough from old one AND if new target if far enough from player
            if (getNorm([this.targetPos[0] - cursorInWorld[0], this.targetPos[1] - cursorInWorld[1]]) > 0.1
                && getNorm([cursorInWorld[0] - this.pos[0], cursorInWorld[1] - this.pos[1]]) > 0.01) {
                this.targetPos = [cursorInWorld[0], cursorInWorld[1]];
                this.vel = normalize([this.targetPos[0] - this.pos[0], this.targetPos[1] - this.pos[1]]);
                this.move = true;
                this.nbMove += 1;
                syncNeeded = true;
                let self = this;
                let nbMove = this.nbMove;
                window.setTimeout(function () {
                    console.log(`stop ${nbMove}?`);
                    self.stop(nbMove)
                }, getNorm([this.targetPos[0] - this.pos[0], this.targetPos[1] - this.pos[1]]) * 1000);
            }
        }

        if (this.move && Math.abs(this.targetPos[0] - this.pos[0]) < 0.011
            && Math.abs(this.targetPos[1] - this.pos[1]) < 0.1) {
            stop(this.nbMove);
        }

        this.pos = [this.pos[0] + this.vel[0] * d, this.pos[1] + this.vel[1] * d];
        super.update();
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
