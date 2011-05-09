var audio = require('./build/default/nodepa.node');

audio.play('test.wav', function(err) {
  console.log(err);
});
/*
setTimeout(function() {
  audio.play('test.wav');
}, 1*1000)

setTimeout(function() {
  audio.play('test.wav');
}, 2*1000)*/

setInterval(function() {
 audio.play('kick.wav');
}, 500)
