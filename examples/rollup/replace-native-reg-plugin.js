import fs from 'node:fs';
import { fileURLToPath } from 'node:url';
import MagicString from 'magic-string';
import nodeGypBuild from 'node-gyp-build';

export default function replaceNativeReg() {
  const nativeRegPath = fs.readlinkSync(fileURLToPath(import.meta.resolve('native-reg/')), 'utf8');
  return {
    name: 'replace-native-reg',

    /**
     * @param {string} code
     * @param {string} id
     */
    transform(code, id) {
      if (!id.endsWith(`${nativeRegPath}lib\\index.js`)) {
        return null;
      }
      // This is the code in that file that tries to load the native module.
      const value = 'require(\'node-gyp-build\')(__dirname + "/..")';
      const index = code.indexOf(value);
      if (index < 0) {
        this.error('could not find expected code in native-reg');
        return null;
      }

      // Lookup the native module (for the current node architecture) and emit it as an asset.
      // You will need to use some other method if you are bundling for another architecture,
      // node-gyp-build does not directly support overriding the target architecture.
      const addonPath = nodeGypBuild.resolve(nativeRegPath);
      const source = fs.readFileSync(addonPath);
      const resourceId = this.emitFile({
        type: 'asset',
        name: 'native-reg.node',
        source,
      });

      // Replace the native module loading code with a reference to the emitted asset, using magic-string to
      // preserve source maps.
      // Code is a bit funky because we are translating a commonjs module that will be translated to esm.
      const magic = new MagicString(code, { filename: id });
      magic.prepend(
        `const { createRequire } = require('node:module');\n` +
        `const { fileURLToPath } = require('node:url');\n` +
        `const nodeRequire = createRequire(import.meta.url);\n`,
      );
      const replacement = `nodeRequire(fileURLToPath(import.meta.ROLLUP_FILE_URL_${resourceId}))`;
      magic.update(index, index + value.length, replacement);
      return { code: magic.toString(), map: magic.generateMap({ hires: true }) };
    },
  };
}