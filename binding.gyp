{
    "targets": [
        {
            "target_name": "kcp",
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "<!(node -p \"require('node-addon-api').include_dir\")"
            ],
            "sources": [
                "src/kcp/ikcp.c",
                "src/kcpobject.cc",
                "src/node-kcp.cc",
                "src/kcp_wrap.cpp"
            ],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            "cflags": ["-Wall", "-O3"],
            "defines": ["USE_NAPI", "NAPI_DISABLE_CPP_EXCEPTIONS"]
        }
    ]
}
