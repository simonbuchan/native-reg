import reg from "native-reg";

// A simple example of a function that uses the native-reg package in a bundled application.
// Returns the value of the specified environment variable that a process started
// by explorer would get, even if the value was changed after the current process started.
export function getCurrentEnv(name) {
  return reg.getValue(reg.HKCU, 'Environment', name, reg.GetValueFlags.NO_EXPAND);
}

export function invalidUse() {
  return reg.getValue("HKCU", 'Environment', 'TEMP');
}
