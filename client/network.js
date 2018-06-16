let socket;
let opened = false;
let syncNeeded = false;

let syncTime = 0.11;

function updatePlayerFromServer(id, pos, vel) {
    console.log(`Update player from server: id=${id}, pos=(${pos.x},${pos.y}) vel=(${vel.x},${vel.y})`);
    player.id = id;
    //player.pos = pos;
    //player.vel = vel;
}

function initNetwork() {
    if (typeof port === 'undefined' || port === null)
        port = 2000;
    socket = new WebSocket("ws://" + window.location.host + ":" + port);

    socket.onopen = function (event) {
        //socket.send("Here's some text that the server is urgently awaiting!");
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

    socket.onmessage = function (event) {
        ////console.log(event.data);
        //let orders = JSON.parse(event.data);
        ////console.log(order);

        //for (j in orders) {

        //    let order = orders[j];

            let order = JSON.parse(event.data);

            if (order.order === "state") {
                if (order.suborder === "entities") {
                    for (i in order.data) {

                        id = order.data[i].id;

                        //console.log(order.data[i]);

                        newPos = [order.data[i].pos.x, order.data[i].pos.y];
                        newVel = [order.data[i].vel.x, order.data[i].vel.y];
                        if (entities.hasOwnProperty(id)) {
                            //console.log(id.toString() + " IS in game with pos " + entities[id].pos.toString() + " and newPos " + newPos.toString());
                            entities[id].pos = newPos;
                        }
                        else {
                            //console.log(id.toString() + " is NOT in game with pos " + newPos.toString());

                            if (order.data[i].type === "player") {
                                if (id !== player.id)
                                    entities[id] = new Entity(newPos, "knight.png");
                                else if (debug)
                                    entities[id] = new Entity(newPos, "debug64r.png");
                            }
                            else if (order.data[i].type === "npc_ally_1")
                                entities[id] = new Entity(newPos, "guard.png");
                            else if (order.data[i].type === "npc_enemy_1")
                                entities[id] = new Entity(newPos, "skeleton_warrior.png");
                            else if (order.data[i].type === "object1")
                                entities[id] = new Entity(newPos, "object1.png", "object1");
                            else {
                                console.log("Unknown type: " + order.data[i].type)
                                entities[id] = new Entity(newPos, "");
                            }

                        }

                        if (entities.hasOwnProperty(id))
                            entities[id].vel = newVel;

                        if (id === player.id) {
                            // FORCE PLAYER POS SYNC
                            //player.pos = newPos;
                        }
                    }
                }
                else if (order.suborder === "player") {
                    updatePlayerFromServer(order.id, order.pos, order.vel);
                }
            }
            else if (order.order === "remove") {
                //console.log(order);
                for (i in order.ids) {
                    if (!entities.hasOwnProperty(order.ids[i]))
                        console.log(`WARNING: REMOVE ORDER OF AN UNKNOWN ID ${order.ids[i].toString()}`);
                    //console.log(entities.hasOwnProperty(order.ids[i]))
                    //console.log(entities.hasOwnProperty(87454654))
                    //console.log(Object.keys(entities).length);
                    delete entities[(order.ids[i])];
                    //console.log(Object.keys(entities).length);
                }
            }

    }
}

function sync(d) {
    syncTime += d;
    if (!opened || !syncNeeded || syncTime < 0.1)
        return;
    syncTime = 0;
    console.log("Syncing...");
    var msg = {
        order: "state",
        suborder: "player",
        speed: 1.0,
        vel: { x: player.vel[0], y: player.vel[1] },
    };
    if (player.targetPos !== undefined)
        msg['targetPos'] = { x: player.targetPos[0], y: player.targetPos[1] };

    socket.send(JSON.stringify(msg, function (key, value) {
        // limit precision of floats
        if (typeof value === 'number') {
            return parseFloat(value.toFixed(3));
        }
        return value;
    }));
    syncNeeded = false;
}
