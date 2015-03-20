{
  "targets": [
    {
      "target_name": "ts",
      "include_dirs": [
        "<!(node -e \"require('nan')\")",
        "/usr/include"
      ],
      "sources": [ "src/touchscreen.cc" ],
      "conditions": [
        ['OS=="linux"', {
          "libraries": [
            "<!@(pkg-config tslib-0.0 --libs)"
          ]
        }]
      ]
    }
  ]
}
