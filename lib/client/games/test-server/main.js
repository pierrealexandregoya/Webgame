let sprites = {
    player: "knight.png",
    npc_ally_1: "guard.png",
    npc_enemy_1: "skeleton_warrior.png",
    object1: "object1.png",
};

function gameInit() {
    player = new Player;
    entities[-1] = player;
    camera = new DebugCamera()
    entities[-2] = camera;
}

class Player extends Entity {
    constructor() {
        super([0, 0], "games/test-server/knight.png");
        this.orders = [];
        this.moving = false;
        this.lastTargetPos = this.pos;
    }

    init(order) {
        this.id = order.id;
        this.setPos(order.pos.x, order.pos.y);
        this.vel = [order.dir.x, order.dir.y];
        this.speed = order.speed;
        camera.pos = player.pos;
    }

    updateFromServer(order) {
        this.pos = [order.pos.x, order.pos.y];
        this.dir = [order.dir.x, order.dir.y];
        this.speed = order.speed;
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

        if (leftMouseDown && this.moving && dist(this.pos, targetPos) < 0.1) {
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
