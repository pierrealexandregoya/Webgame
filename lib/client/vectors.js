function dotProduct(array1, array2) {
    let r = 0;
    for (i in array1)
        r += array1[i] * array2[i];
    return r;
}

function getNorm(array) {
    let tot = 0;

    for (i in array) {
        tot += array[i] * array[i];
    }
    return Math.sqrt(tot);
}

function normalize(array) {
    let norm = getNorm(array);
    let r = array
    if (norm !== 0) {
        r[0] /= norm;
        r[1] /= norm;
    }
    return r;
}

function dist(vec1, vec2) {
    return getNorm([vec1[0] - vec2[0]], [vec1[1] - vec2[1]]);
}
