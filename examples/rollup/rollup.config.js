import { defineConfig } from 'rollup';
import sourcemaps from "@gordonmleigh/rollup-plugin-sourcemaps";
import nodeResolve from '@rollup/plugin-node-resolve';
import commonjs from '@rollup/plugin-commonjs';
import replaceNativeReg from './replace-native-reg-plugin.js';

const methodConfigMap = {
  // This is by far the easiest way to bundle native modules.
  external: {
    external: ['native-reg'],
  },
  // Manually replace the `node-gyp-build` package usage with a custom plugin that copies the addon as an asset.
  // This is a lot more complex, but lets you completely control how the native module is bundled.
  replace: {
    plugins: [
      sourcemaps(),
      replaceNativeReg(),
      nodeResolve(),
      commonjs(),
    ],
  },
};

const methodConfig = methodConfigMap[process.env.METHOD];
if (!methodConfig) {
  throw new Error(`unknown method: ${process.env.METHOD}, expected one of ${Object.keys(methodConfigMap).join(', ')}`);
}

export default defineConfig({
  input: 'src/index.js',
  output: {
    file: 'dist/index.js',
    sourcemap: true,
    format: 'es',
  },
  ...methodConfig,
});
