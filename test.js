var kcp = require('./build/Release/kcp');
var kcpobj1 = new kcp.KCP(123);
var kcpobj2 = new kcp.KCP(123);
kcpobj1.output(function(data, size){
    kcpobj2.input(data);
});
kcpobj2.output(function(data){
	kcpobj1.input(data);
});
var packageindex = 1;
setInterval(function(){
    kcpobj1.update(Date.now());
    kcpobj2.update(Date.now());
    kcpobj1.send('hello world'+(packageindex++));
}, 600);
setInterval(function(){
    var data2 = kcpobj2.recv();
    if (data2) {
        console.log("RECV FROM KCP-1: "+data2);
        kcpobj2.send(data2);
    }
    var data1 = kcpobj1.recv();
    if (data1) {
    	console.log("RECV FROM KCP-2: "+data1);
    }
}, 200);

