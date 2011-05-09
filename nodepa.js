var PAudio = require('./build/default/nodepa.node').PAudio,
    audio = new PAudio();

audio.play('test.wav');

setInterval(function() {
 audio.play('kick.wav');
}, 500)
