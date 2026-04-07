import { mkdir, copyFile } from 'node:fs/promises';
import path from 'node:path';

const root = path.resolve(process.cwd(), '..');
const buildDir = path.join(root, 'platform', 'web', 'wasm', 'build');
const targetDir = path.join(process.cwd(), 'public', 'wasm');

await mkdir(targetDir, { recursive: true });
await copyFile(path.join(buildDir, 'clouds_wasm_engine.js'), path.join(targetDir, 'clouds_wasm_engine.js'));
await copyFile(path.join(buildDir, 'clouds_wasm_engine.wasm'), path.join(targetDir, 'clouds_wasm_engine.wasm'));
console.log('WASM artifacts copied to web/public/wasm');
