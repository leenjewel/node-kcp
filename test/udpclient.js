var kcp = require('./../build/Release/kcp');
var kcpobj = new kcp.KCP(123, {address: '127.0.0.1', port: 41234});
var dgram = require('dgram');
var client = dgram.createSocket('udp4');
var msg = JSON.stringify({
    id: 'test',
    route: 'test',
    body: 'test'
});
var idx = 1;
var interval = 200;

kcpobj.stream(1);
kcpobj.nodelay(0, interval, 0, 0);

kcpobj.output((data, size, context) => {
    client.send(data, 0, size, context.port, context.address);
});

client.on('error', (err) => {
    console.log(`client error:\n${err.stack}`);
    client.close();
});

client.on('message', (data, rinfo) => {
    kcpobj.input(data);
    var recv = kcpobj.recv();
    if (recv) {
    	console.log(`Client recv ${recv} from ${kcpobj.context().address}:${kcpobj.context().port}`);
        kcpobj.send(msg+(idx++));
    }
});

setInterval(() => {
    kcpobj.update(Date.now());
}, interval);

kcpobj.send(msg);
