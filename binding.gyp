{
    "targets": [
        {
            "target_name": "kcp",
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            "sources": [
                "src/kcp/ikcp.c",
                "src/kcpobject.cc",
                "src/node-kcp.cc"
            ],
            "defines": ["USE_NAPI"]
        }
    ]
}
