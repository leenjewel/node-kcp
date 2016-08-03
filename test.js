var kcp = require('./build/Release/kcp');
var kcpobj1 = new kcp.KCP(123);
var kcpobj2 = new kcp.KCP(123);
kcpobj1.output(function(data, size){
    console.log("SIZE = "+data.length);
    console.log("SEND("+size+"): "+kcpobj2.input(data));
});
kcpobj2.output(function(data){
    console.dir(data);
});
var packageindex = 1;
setInterval(function(){
    kcpobj1.update(Date.now());
    kcpobj2.update(Date.now());
    kcpobj1.send('hello world'+(packageindex++));
}, 600);
setInterval(function(){
    var data = kcpobj2.recv();
    if (data) {
        console.dir(data);
    }
}, 200);

