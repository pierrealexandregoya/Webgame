class Player extends Entity {
    constructor(pos) {
        super(pos, "knight.png", "player");
        //this.move = false;
        //this.nbMove = 0;
        this.orders = [];
        this.moving = false;
    }

    //stop(nbMove) {
    //    if (nbMove !== this.nbMove)
    //        return;
    //    //console.log("stop");
    //    this.vel = [0, 0]
    //    this.move = false;
    //    this.targetPos = undefined;
    //    syncNeeded = true;
    //}

    changeSpeed(newSpeed) {
        console.log("PUSHING change_speed action");
        this.orders.push({ order: "action", suborder: "change_speed", speed: newSpeed });
    }

    changeDir(newDir) {
        console.log("PUSHING change_dir action");
        this.orders.push({ order: "action", suborder: "change_dir", dir: { x: newDir[0], y: newDir[1] } });
    }

    moveTo(targetPos) {
        console.log("PUSHING move_to action");
        this.orders.push({ order: "action", suborder: "move_to", target_pos: { x: targetPos[0], y: targetPos[1] } });
    }

    update(d) {
        //if (mouseDown) {
        //    let cursorInWorld = screenToWorld(mousePos);
        //    // If new target if far enough from old one (if it exists) AND if new target if far enough from player
        //    if ((this.targetPos === undefined || getNorm([this.targetPos[0] - cursorInWorld[0], this.targetPos[1] - cursorInWorld[1]]) > 0.1)
        //        && getNorm([cursorInWorld[0] - this.pos[0], cursorInWorld[1] - this.pos[1]]) > 0.01) {
        //        this.targetPos = [cursorInWorld[0], cursorInWorld[1]];
        //        //this.vel = normalize([this.targetPos[0] - this.pos[0], this.targetPos[1] - this.pos[1]]);
        //        this.move = true;
        //        this.nbMove += 1;
        //        syncNeeded = true;
        //        let self = this;
        //        let nbMove = this.nbMove;
        //        //window.setTimeout(function () {
        //        //    //console.log(`stop ${nbMove}?`);
        //        //    self.stop(nbMove)
        //        //}, getNorm([this.targetPos[0] - this.pos[0], this.targetPos[1] - this.pos[1]]) * 1000);
        //    }
        //}

        ////if (this.move && Math.abs(this.targetPos[0] - this.pos[0]) < 0.011
        ////    && Math.abs(this.targetPos[1] - this.pos[1]) < 0.1) {
        ////    stop(this.nbMove);
        ////}

        ////this.pos = [this.pos[0] + this.vel[0] * this.speed * d, this.pos[1] + this.vel[1] * this.speed * d];

        let cursorInWorld = screenToWorld(mousePos)
        let targetPos = [cursorInWorld[0], cursorInWorld[1]];

        if (leftMouseDown) {
            if (!this.moving) {
                this.changeSpeed(1);
                //console.log("==");
                this.dir = [targetPos[0] - this.pos[0], targetPos[1] - this.pos[1]];
                this.changeDir(this.dir);
                this.moving = true;
            }
            else {
                //console.log("--");
                let dir = [targetPos[0] - this.pos[0], targetPos[1] - this.pos[1]];
                if (dir != this.dir) {
                    this.dir = dir;
                    this.changeDir(this.dir);
                }
            }
        }

        if (!leftMouseDown && this.moving) {
            this.changeSpeed(0);
            this.moving = false;
        }

        if (rightMouseClicked) {
            this.moveTo(targetPos);
        }

        super.update(d);
    }
}
