class Player extends Entity {
    constructor(pos) {
        super(pos, "knight256.png");
        this.move = false;
        this.nbMove = 0;
    }

    stop(nbMove) {
        if (nbMove !== this.nbMove)
            return;
        //console.log("stop");
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
                    //console.log(`stop ${nbMove}?`);
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
