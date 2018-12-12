let socket;
let opened = false;
let syncNeeded = false;

let syncTime = 0.11;

function makeid() {
    var text = "";
    var possible = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    for (var i = 0; i < 5; i++)
        text += possible.charAt(Math.floor(Math.random() * possible.length));

    return text;
}

function updatePlayerFromServer(pos, dir, speed) {
    //if (debug)
    //    console.log(`Update player from server: pos=(${pos[0]},${pos[1]}) vel=(${dir[0]},${dir[1]})`);

    player.pos = pos;
    player.dir = dir;
    player.speed = speed;
}

let sprites = {
    player: "knight.png",
    npc_ally_1: "guard.png",
    npc_enemy_1: "skeleton_warrior.png",
    object1: "object1.png",
};

function onOrder(event) {
    let order = JSON.parse(event.data);

    //if (debug)
    //    console.log(order);

    if (order.order === "state") {
        if (order.suborder === "entities") {
            for (i in order.data) {

                let entityState = order.data[i];

                if (entityState.id === player.id) {
                    console.log("ERROR: got our id in other entities' state");
                    continue;
                }

                let pos = [entityState.pos.x, entityState.pos.y];

                if (!entities.hasOwnProperty(entityState.id)) {
                    let sprite = sprites[entityState.type];
                    if (!sprite) {
                        console.log("ERROR: unknown type: " + entityState.type);
                        sprite = "";
                    }
                    entities[entityState.id] = new Entity(pos, sprite);
                }
                else
                    entities[entityState.id].pos = pos;

                if ('dir' in entityState)
                    entities[entityState.id].dir = [entityState.dir.x, entityState.dir.y];

                if ('speed' in entityState)
                    entities[entityState.id].speed = entityState.speed;

                if ('type' in entityState)
                    entities[entityState.id].type = entityState.type;
            }
        }
        else if (order.suborder === "player") {
            updatePlayerFromServer([order.pos.x, order.pos.y], [order.dir.x, order.dir.y], order.speed);
        }
    }
    else if (order.order === "remove") {
        for (i in order.ids) {
            if (!entities.hasOwnProperty(order.ids[i]))
                console.log(`WARNING: REMOVE ORDER OF AN UNKNOWN ID ${order.ids[i].toString()}`);
            delete entities[(order.ids[i])];
        }
    }

}

function initNetwork() {
    if (typeof port === 'undefined' || port === null)
        port = 2000;
    socket = new WebSocket("ws://" + window.location.host + ":" + port);

    socket.onopen = function (event) {
        send({ order: "authentication", player_name: "killer69" });
        socket.onmessage = function (event) {
            let order = JSON.parse(event.data);
            console.log(`Got game state, tick duration is ${order.tick_duration}`);
            socket.onmessage = function (event) {
                let order = JSON.parse(event.data);
                player.id = order.id;
                player.pos = [order.pos.x, order.pos.y];
                player.vel = [order.dir.x, order.dir.y];
                player.speed = order.speed;
                console.log(`Got player state, id=${order.id}, pos=${player.pos}, dir=${player.vel}, speed=${order.speed}, max_speed=${order.max_speed}`);
                camera.pos = player.pos.slice();
                socket.onmessage = onOrder;
            }
        }
        opened = true;
    };

    socket.onclose = function (event) {
        opened = false;
        let str = "Socket closed";
        if (event.code === 1006)
            str += ": abnormal";
        else if (event.reason !== "")
            str += ": " + event.reason;
        console.log(str);
    }

    socket.onerror = function (event) {
        console.log("Socket error");
    }
}

function send(order) {
    if (debug)
        console.log(`SENDING ${JSON.stringify(order)}`);

    socket.send(JSON.stringify(order, function (key, value) {
        // limit precision of floats
        if (typeof value === 'number') {
            return parseFloat(value.toFixed(3));
        }
        return value;
    }));
}

function sync(d) {
    if (!opened)
        return;

    while (player.orders.length > 0) {
        send(player.orders.shift());
    }
}
