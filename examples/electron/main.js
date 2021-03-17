const reg = require("native-reg");

const { app, BrowserWindow } = require("electron");

app.on("window-all-closed", () => {
    app.exit();
});

app.whenReady().then(async () => {
    const raw = reg.getValue(reg.HKCU, 'Environment', 'TEMP', reg.GetValueFlags.NO_EXPAND);
    console.log("%TEMP%=%s", raw);

    const window = new BrowserWindow({
        webPreferences: {
            preload: require.resolve("./preload"),
        },
    });
    window.webContents.openDevTools({ mode: "right" });

    window.loadFile("window.html");
});
