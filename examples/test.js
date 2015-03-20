var touchscreen = require("../index");

var touchCount = 0;

touchscreen("/dev/input/touchscreen", function(err, data) {
    if (err) {
        throw err;
    }


    console.log(data);
});
