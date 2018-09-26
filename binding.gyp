{
  "targets": [
    {
      "target_name": "reg",
      "sources": [ "reg.cc" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ]
    }
  ]
}
