let sprites = {
    mineral: "mineral.png",
};

function gameInit() {
    player = new Player;
    entities[-1] = player;
    camera = new DebugCamera()
    entities[-2] = camera;
}

class Player extends Entity {
    constructor() {
        super([0, 0]);
        this.orders = [];
    }

    updateFromServer() {

    }

    init(order) {
        this.id = order.id;
        this.setPos(order.pos.x, order.pos.y);
        //console.log(`Got player state, id=${order.id}, pos=${player.pos}`);
        //camera.pos = player.pos.slice();
        camera.pos = player.pos;
    }
}
