{
  "targets": [
    {
      "target_name": "fastreg",
      "sources": [ "fastreg.cc" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
