function initNetwork() {
    var exampleSocket = new WebSocket("ws://" + window.location.host + ":2000");

    exampleSocket.onopen = function (event) {
        exampleSocket.send("Here's some text that the server is urgently awaiting!");
    };

    exampleSocket.onmessage = function (event) {
        order = JSON.parse(event.data);
        for (i in order.data) {

            id = order.data[i].id;

            //console.log(order.data[i]);

            newPos = [order.data[i].pos.x, order.data[i].pos.y];
            newVel = [order.data[i].vel.x, order.data[i].vel.y];
            if (entities.hasOwnProperty(id)) {
                //console.log(id.toString() + " is in game with pos " + entities[id].pos.toString() + " and newPos " + newPos.toString());
                entities[id].pos = newPos;
            }
            else {
                if (order.data[i].type === "enemy1")
                    entities[id] = new Entity(newPos, "knight64.png");
                else if (order.data[i].type === "object1")
                    entities[id] = new Entity(newPos, "cross16.png");
                else {
                    console.log("Unknown type: " + order.data[i].type)
                    entities[id] = new Entity(newPos, "");
                }

            }
            entities[id].vel = newVel;

        }
    }
}
