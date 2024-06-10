const { contextBridge } = require("electron");
contextBridge.exposeInMainWorld("reg", require("native-reg"));