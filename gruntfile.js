'use strict';

module.exports = function(grunt) {

    grunt.loadNpmTasks('grunt-mocha-test');
    grunt.loadNpmTasks('grunt-contrib-clean');

    var src = ["./test/test.js"];

    grunt.initConfig({
        mochaTest: {
            test: {
                options: {
                    reporter: "spec",
                    timeout: 5000,
                    require: ""
                },
                src: src
            }
        },
        clean: {
            "coverage.html": {
                src: ["coverage.html"]
            }
        }
    });

    grunt.registerTask('default', ['clean', 'mochaTest']);
}
