#node-kcp

[KCP Protocol](https://github.com/skywind3000/kcp) for Node.js

##HowTo

###Build:

```
npm install -g node-gyp

node-gyp configure

git clone git@github.com:leenjewel/node-kcp

cd node-kcp

git submodule init

git submodule update

node-gyp build
```

###Run Example:

Run UDP server in a shell

```
cd node-kcp

node udpserver.js
```

And run UDP client in another shell

```
cd node-kcp

node udpclient.js
```