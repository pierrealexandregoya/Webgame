class Player extends Entity {
    constructor(pos) {
        super(pos, "knight.png", "player");
        this.orders = [];
        this.moving = false;
        this.lastTargetPos = this.pos;
    }

    changeSpeed(newSpeed) {
        this.orders.push({ order: "action", suborder: "change_speed", speed: newSpeed });
    }

    changeDir(newDir) {
        this.orders.push({ order: "action", suborder: "change_dir", dir: { x: newDir[0], y: newDir[1] } });
    }

    moveTo(targetPos) {
        this.orders.push({ order: "action", suborder: "move_to", target_pos: { x: targetPos[0], y: targetPos[1] } });
    }

    update(d) {
        let cursorInWorld = screenToWorld(mousePos)
        let targetPos = [cursorInWorld[0], cursorInWorld[1]];

        if (leftMouseDown && this.moving && dist(this.pos, targetPos) < 0.1)
        {
            this.changeSpeed(0);
            this.moving = false;
        }
        else if (leftMouseDown && dist(this.lastTargetPos, targetPos) > 0.01) {

            if (!this.moving) {
                this.changeSpeed(1);
                this.dir = [targetPos[0] - this.pos[0], targetPos[1] - this.pos[1]];
                this.changeDir(this.dir);
                this.moving = true;
            }
            else {
                let dir = [targetPos[0] - this.pos[0], targetPos[1] - this.pos[1]];
                if (dir != this.dir) {
                    this.dir = dir;
                    this.changeDir(this.dir);
                }
            }
            this.lastTargetPos = targetPos;

        }

        if (!leftMouseDown && this.moving) {
            this.changeSpeed(0);
            this.moving = false;
        }

        if (rightMouseClicked) {
            this.changeSpeed(1);
            this.moveTo(targetPos);
        }

        super.update(d);
    }
}
