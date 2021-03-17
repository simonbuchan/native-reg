const reg = require("native-reg");

const [execPath, scriptPath, command, ...args] = process.argv;

const hive = reg.HKCU;
const runSubKey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
const runValueName = "native-reg-example";

// This quoting works so long as none of these end in a backslash. That's fine for this example.
// See the actual rules here: https://devblogs.microsoft.com/oldnewthing/20100917-00/?p=12833
const runString = [execPath, scriptPath, "print", ...args]
    .map(value => `"${value}"`)
    .join(" ");

switch (command) {
    default:
        console.error("invalid command: %O", command);
        break;
    case "register": {
        const hkey = reg.openKey(hive, runSubKey, reg.Access.ALL_ACCESS);
        if (!hkey) {
            throw error("could not open autorun key: %O", runSubKey);
        }
        try {
            reg.setValueSZ(hkey, runValueName, runString);
        } finally {
            reg.closeKey(hkey);
        }
        break;
    }
    case "unregister": {
        const hkey = reg.openKey(hive, runSubKey, reg.Access.ALL_ACCESS);
        if (!hkey) {
            throw error("could not open autorun key: %O", runSubKey);
        }
        try {
            reg.deleteValue(hkey, runValueName, runString);
        } finally {
            reg.closeKey(hkey);
        }
        break;
    }
    case "print":
        console.log(...args);
        console.log("press any key");
        process.stdin.setRawMode(true);
        process.stdin.on("data", () => {
            process.exit();
        });
        break;
}
