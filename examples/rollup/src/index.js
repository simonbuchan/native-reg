import { getCurrentEnv, invalidUse } from "./env.js";
console.log("%TEMP%=%s", getCurrentEnv("TEMP"));
console.log("stack: %s", new Error().stack);
invalidUse();
