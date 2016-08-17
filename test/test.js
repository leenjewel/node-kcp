var kcp = require('./../build/Release/kcp');
var expect = require('chai').expect;

var kcpobj1 = new kcp.KCP(123, {name : 'kcpobj1'});
var kcpobj2 = new kcp.KCP(123, {name : 'kcpobj2'});

var interval = 10;
var msg = 'helloworld';

describe('Test node-kcp', function(){

    describe('# nodelay', function(){
        it('set default mode', function(){
            expect(kcpobj1.nodelay(0, interval, 0, 0)).to.be.equal(0);
            expect(kcpobj2.nodelay(0, interval, 0, 0)).to.be.equal(0);
        });
    });

    describe('# update & check', function(){
        it('test update and check', function(){
            var now = Date.now();
            kcpobj1.update(now);
            expect(kcpobj1.check(now)).to.be.equal(interval);
            kcpobj2.update(now);
            expect(kcpobj2.check(now)).to.be.equal(interval);
        });
    });

    describe('# input & output', function(){
        it('test input and output', function(done){
            var kcpobj1TID = setTimeout(function(){
                kcpobj1.update(Date.now());
            }, interval);
            kcpobj1.output(function(data, size, context){
                expect(context.name).to.be.equal('kcpobj1');
                expect(kcpobj2.input(data)).to.be.equal(0);
                clearTimeout(kcpobj1TID);
                done();
            });
            kcpobj1.send(msg);
        });
    });

    describe('# recveive message', function(){
        it('test receive msg', function(done){
            var kcpobj2TID = setTimeout(function(){
                kcpobj2.update(Date.now());
                expect(kcpobj2.recv().toString()).to.be.equal(msg);
                done();
            }, interval);
        });
    });

});

