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
