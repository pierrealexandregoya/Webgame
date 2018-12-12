class Entity {
    constructor(pos, texUrl, type) {
        this.pos = pos;
        this.targetPos = pos;
        this.texUrl = texUrl;
        this.dir = [0, 0];
        this.speed = 0;
        this.rot = [0, 1];
        this.angle = 0;
        this.scale = [1, 1];
        this.texGl = loadTexture(gl, this.texUrl);
        this.buffers = initBuffers(gl);
        if (typeof type === 'undefined') {
            this.type = "";
        }
        else
            this.type = type;
    }

    update(d) {
        this.pos[0] = this.pos[0] + this.dir[0] * this.speed * d;
        this.pos[1] = this.pos[1] + this.dir[1] * this.speed * d;

        if (Math.abs(getNorm(this.dir)) > Number.EPSILON) {
            this.rot[1] = dotProduct(this.dir, [0, 1]) / (getNorm(this.dir) * getNorm([0, 1]));
            this.rot[0] = Math.sqrt(1 - this.rot[1] * this.rot[1]);
            if (this.dir[0] < 0)
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
