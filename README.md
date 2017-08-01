node-kcp
======================================

[![Build Status][1]][2]

[1]: https://api.travis-ci.org/leenjewel/node-kcp.svg?branch=master
[2]: https://travis-ci.org/leenjewel/node-kcp


[KCP Protocol](https://github.com/skywind3000/kcp) for Node.js

## HowTo

### Build:

```
npm install -g node-gyp

node-gyp configure

git clone git@github.com:leenjewel/node-kcp

cd node-kcp

git submodule init

git submodule update

node-gyp build
```

## Example:

### Install by npm

```
npm install node-kcp
```

### udpserver.js

```
var kcp = require('node-kcp');
var dgram = require('dgram');
var server = dgram.createSocket('udp4');
var clients = {};
var interval = 200;

var output = function(data, size, context) {
    server.send(data, 0, size, context.port, context.address);
};

server.on('error', (err) => {
    console.log(`server error:\n${err.stack}`);
    server.close();
});

server.on('message', (msg, rinfo) => {
    var k = rinfo.address+'_'+rinfo.port;
    if (undefined === clients[k]) {
        var context = {
            address : rinfo.address,
            port : rinfo.port
        };
        var kcpobj = new kcp.KCP(123, context);
        kcpobj.nodelay(0, interval, 0, 0);
        kcpobj.output(output);
        clients[k] = kcpobj;
    }
    var kcpobj = clients[k];
    kcpobj.input(msg);
});

server.on('listening', () => {
    var address = server.address();
    console.log(`server listening ${address.address} : ${address.port}`);
    setInterval(() => {
        for (var k in clients) {
            var kcpobj = clients[k];
        	kcpobj.update(Date.now());
        	var recv = kcpobj.recv();
        	if (recv) {
            	console.log(`server recv ${recv} from ${kcpobj.context().address}:${kcpobj.context().port}`);
           		kcpobj.send('RE-'+recv);
       	 	}
       	}
    }, interval);
});

server.bind(41234);

```

### udpclient.js

```
var kcp = require('node-kcp');
var kcpobj = new kcp.KCP(123, {address: '127.0.0.1', port: 41234});
var dgram = require('dgram');
var client = dgram.createSocket('udp4');
var msg = 'hello world';
var idx = 1;
var interval = 200;

kcpobj.nodelay(0, interval, 0, 0);

kcpobj.output((data, size, context) => {
    client.send(data, 0, size, context.port, context.address);
});

client.on('error', (err) => {
    console.log(`client error:\n${err.stack}`);
    client.close();
});

client.on('message', (msg, rinfo) => {
    kcpobj.input(msg);
});

setInterval(() => {
    kcpobj.update(Date.now());
    var recv = kcpobj.recv();
    if (recv) {
        console.log(`client recv ${recv}`);
        kcpobj.send(msg+(idx++));
    }
}, interval);

kcpobj.send(msg+(idx++));

```

## About Pomelo and Pomelo-kcp

If you want to use [node-kcp](https://github.com/leenjewel/node-kcp) in [pomelo](https://github.com/NetEase/pomelo/) server, you need [pomelo-kcp](https://github.com/leenjewel/pomelo-kcp)