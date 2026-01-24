import embeddedSimulideWasmBase64 from '../wasm/lceda-pro-sim-server.wasm';
import simulideScriptText from '../wasm/lceda-pro-sim-server.js?raw';

const SIMULIDE_JS = 'wasm/lceda-pro-sim-server.js';
const SIMULIDE_WASM = 'wasm/lceda-pro-sim-server.wasm';
const DEBUG = true;

type CCallReturnType = 'number' | 'string' | 'boolean' | 'array' | 'null' | null;

type SimulideModule = {
	ccall: (ident: string, returnType: CCallReturnType, argTypes: string[], args: any[]) => any;
	_loadCircuitFromFile: (filenamePtr: number, contentPtr: number) => number;
	_startSimulation: () => void;
	_stopSimulation: () => void;
	_pauseSimulation: () => void;
	_resumeSimulation: () => void;
	_getSimulationState: () => number;
	_setSimulationSpeed: (speed: number) => void;
	_getSimulationTime: () => number;
	_stepSimulation: () => void;
	_updateCircuitData: (updateCirIdPtr: number, attrInputPtr: number, updateValuePtr: number) => number;
	_getSimulationData: () => number;
	_getVersion: () => number;
	_main: (argc: number, argv: number) => number;
	_fflush: (streamPtr: number) => number;
	_free: (ptr: number) => void;
	_emscripten_stack_get_end: () => number;
	_emscripten_stack_get_base: () => number;
	_strerror: (err: number) => number;
	_setThrew: (threw: number, value: number) => void;
	__emscripten_tempret_set: (value: number) => void;
	_emscripten_stack_init: () => void;
	_emscripten_stack_get_free: () => number;
	__emscripten_stack_restore: (stack: number) => void;
	__emscripten_stack_alloc: (size: number) => number;
	_emscripten_stack_get_current: () => number;
	___cxa_free_exception: (ptr: number) => void;
	___cxa_decrement_exception_refcount: (ptr: number) => void;
	___cxa_increment_exception_refcount: (ptr: number) => void;
	___get_exception_message: (excPtr: number, ptrPtr: number, lenPtr: number) => number;
	___cxa_can_catch: (thrownType: number, catchType: number, adjustedPtr: number) => number;
	___cxa_get_exception_ptr: (excPtr: number) => number;
	memory: WebAssembly.Memory;
	__indirect_function_table: WebAssembly.Table;
	wasmMemory?: WebAssembly.Memory;
	wasmTable?: WebAssembly.Table;
};

type SimEventType =
	| 'SESSION_START'
	| 'SESSION_PAUSE'
	| 'SESSION_RESUME'
	| 'SESSION_STOP'
	| 'SPEED_SET'
	| 'COMPONENT_UPDATE';

type CircuitPayload = {
	filename?: string;
	content?: string;
};
type ComponentUpdatePayload = {
	updateCirId?: string;
	attrInput?: string;
	updateValue?: string;
};

const SimState = {
	SIM_STOPPED: 0,
	SIM_ERROR: 1,
	SIM_STARTING: 2,
	SIM_PAUSED: 3,
	SIM_WAITING: 4,
	SIM_RUNNING: 5,
	SIM_DEBUG: 6,
} as const;

let cachedModule: SimulideModule | null = null;
let modulePromise: Promise<SimulideModule> | null = null;
let stepTimerId: number | null = null;
let dataTimerId: number | null = null;
let isSimulationRunning = false;
let simulationSpeed = 50;

const DATA_PUSH_INTERVAL_MS = 20;
const MIN_STEP_INTERVAL_MS = 5;
const MAX_STEP_INTERVAL_MS = 100;

function debugLog(message: string, extra?: unknown): void {
	if (!DEBUG) {
		return;
	}
	if (extra !== undefined) {
		console.log(`[simulide][debug] ${message}`, extra);
	} else {
		console.log(`[simulide][debug] ${message}`);
	}
}

function resolveUrl(path: string): string {
	try {
		if (typeof window !== 'undefined' && window.location?.href) {
			return new URL(path, window.location.href).toString();
		}
	} catch {}
	return path;
}

function base64ToBytes(data: string): Uint8Array {
	if (typeof atob === 'function') {
		const binary = atob(data);
		const bytes = new Uint8Array(binary.length);
		for (let i = 0; i < binary.length; i++) {
			bytes[i] = binary.charCodeAt(i);
		}
		return bytes;
	}
	if (typeof Buffer !== 'undefined') {
		return new Uint8Array(Buffer.from(data, 'base64'));
	}
	throw new Error('No base64 decoder available');
}

async function getExtensionFile(path: string): Promise<File | undefined> {
	const sysFs = (globalThis as any)?.eda?.sys_FileSystem;
	if (!sysFs?.getExtensionFile) {
		return undefined;
	}
	let file = await sysFs.getExtensionFile(path);
	if (!file && !path.startsWith('/')) {
		file = await sysFs.getExtensionFile(`/${path}`);
	}
	return file ?? undefined;
}

async function readExtensionBinary(path: string): Promise<Uint8Array | undefined> {
	const file = await getExtensionFile(path);
	if (!file) {
		return undefined;
	}
	const buf = await file.arrayBuffer();
	return new Uint8Array(buf);
}

async function readExtensionText(path: string): Promise<string | undefined> {
	const file = await getExtensionFile(path);
	if (!file) {
		return undefined;
	}
	return await file.text();
}

function createModuleImportUrl(source: string): string {
	const blob = new Blob([source], { type: 'text/javascript' });
	return URL.createObjectURL(blob);
}

async function loadSimulideFactory(): Promise<(opts?: any) => Promise<SimulideModule>> {
	const inlineScript = simulideScriptText || (await readExtensionText(SIMULIDE_JS));
	if (!inlineScript) {
		const url = resolveUrl(SIMULIDE_JS);
		debugLog('Loading simulide module', url);
		const mod = await import(/* @vite-ignore */ url);
		const factory = mod?.default ?? mod;
		if (typeof factory !== 'function') {
			throw new TypeError('Simulide module factory not found');
		}
		return factory as (opts?: any) => Promise<SimulideModule>;
	}
	const blobUrl = createModuleImportUrl(inlineScript);
	debugLog('Loading simulide module from inline script');
	const mod = await import(/* @vite-ignore */ blobUrl);
	URL.revokeObjectURL(blobUrl);
	const factory = mod?.default ?? mod;
	if (typeof factory !== 'function') {
		throw new TypeError('Simulide module factory not found');
	}
	return factory as (opts?: any) => Promise<SimulideModule>;
}

async function ensureSimulideLoaded(): Promise<SimulideModule> {
	if (cachedModule) {
		return cachedModule;
	}
	if (!modulePromise) {
		modulePromise = (async () => {
			const factory = await loadSimulideFactory();
			const wasmBinary
				= (await readExtensionBinary(SIMULIDE_WASM))
				?? (embeddedSimulideWasmBase64 ? base64ToBytes(embeddedSimulideWasmBase64) : undefined);
			const Module = await factory({
				wasmBinary,
				locateFile: (path: string) => resolveUrl(path),
			});
			cachedModule = Module;
			debugLog('Simulide module ready');
			try {
				const version = Module.ccall('getVersion', 'string', [], []);
				debugLog('Simulide version', version);
			} catch (e) {
				debugLog('Simulide version read failed', e);
			}
			return Module;
		})();
	}
	return modulePromise;
}

function extractCircuitPayload(props?: any): CircuitPayload {
	if (!props) {
		return {};
	}
	return {
		filename: props.fileName ?? props.filename ?? props.name ?? 'circuit.sim',
		content: props.fileContent ?? props.content ?? props.netlist ?? props.raw ?? '',
	};
}

function extractComponentUpdate(props?: any): ComponentUpdatePayload {
	if (!props) {
		return {};
	}
	return {
		updateCirId: props.updateCirID ?? props.id ?? props.componentId,
		attrInput: props.attrInput ?? props.attr ?? props.key,
		updateValue: props.updateValueStr ?? props.value ?? props.val,
	};
}

function clampSpeed(speed: number): number {
	if (!Number.isFinite(speed)) {
		return 1;
	}
	return Math.max(1, Math.min(100, Math.round(speed)));
}

function speedToIntervalMs(speed: number): number {
	const s = clampSpeed(speed);
	const range = MAX_STEP_INTERVAL_MS - MIN_STEP_INTERVAL_MS;
	const interval = MAX_STEP_INTERVAL_MS - Math.round((range * (s - 1)) / 99);
	return Math.max(MIN_STEP_INTERVAL_MS, Math.min(MAX_STEP_INTERVAL_MS, interval));
}

function stopTimers(): void {
	if (stepTimerId !== null) {
		clearInterval(stepTimerId);
		stepTimerId = null;
	}
	if (dataTimerId !== null) {
		clearInterval(dataTimerId);
		dataTimerId = null;
	}
	isSimulationRunning = false;
}

function startStepTimer(): void {
	if (stepTimerId !== null) {
		clearInterval(stepTimerId);
	}
	const interval = speedToIntervalMs(simulationSpeed);
	stepTimerId = setInterval(() => {
		if (!cachedModule || !isSimulationRunning) {
			return;
		}
		try {
			const state = cachedModule._getSimulationState();
			if (state === SimState.SIM_RUNNING) {
				cachedModule._stepSimulation();
			}
		} catch (e) {
			stopTimers();
			debugLog('Simulation step error', e);
		}
	}, interval) as unknown as number;
}

function startDataTimer(): void {
	if (dataTimerId !== null) {
		clearInterval(dataTimerId);
	}
	dataTimerId = setInterval(() => {
		if (!cachedModule || !isSimulationRunning) {
			return;
		}
		try {
			const state = cachedModule._getSimulationState();
			if (state !== SimState.SIM_RUNNING) {
				return;
			}
			const dataJson = cachedModule.ccall('getSimulationData', 'string', [], []);
			if (dataJson && dataJson.trim() !== '{}') {
				eda.sch_SimulationEngine.pushData('STREAM_DATA', dataJson);
			}
		} catch (e) {
			stopTimers();
			debugLog('Simulation data error', e);
		}
	}, DATA_PUSH_INTERVAL_MS) as unknown as number;
}

function startTimers(): void {
	if (isSimulationRunning) {
		return;
	}
	isSimulationRunning = true;
	startStepTimer();
	startDataTimer();
}

async function loadCircuitFromFile(payload: CircuitPayload): Promise<number> {
	const Module = await ensureSimulideLoaded();
	const filename = payload.filename ?? 'circuit.sim';
	const content = payload.content ?? '';
	return Module.ccall('loadCircuitFromFile', 'number', ['string', 'string'], [filename, content]);
}

async function startSimulation(): Promise<void> {
	const Module = await ensureSimulideLoaded();
	Module._startSimulation();
	startTimers();
}

async function stopSimulation(): Promise<void> {
	const Module = await ensureSimulideLoaded();
	stopTimers();
	Module._stopSimulation();
}

async function pauseSimulation(): Promise<void> {
	const Module = await ensureSimulideLoaded();
	stopTimers();
	Module._pauseSimulation();
}

async function resumeSimulation(): Promise<void> {
	const Module = await ensureSimulideLoaded();
	Module._resumeSimulation();
	startTimers();
}

async function setSimulationSpeed(speed: number): Promise<void> {
	const nextSpeed = clampSpeed(speed);
	simulationSpeed = nextSpeed;
	if (isSimulationRunning) {
		startStepTimer();
	}
}

async function updateCircuitData(updateCirId: string, attrInput: string, updateValue: string): Promise<number> {
	const Module = await ensureSimulideLoaded();
	return Module.ccall('updateCircuitData', 'number', ['string', 'string', 'string'], [
		updateCirId,
		attrInput,
		updateValue,
	]);
}

async function getSimulationData(): Promise<string> {
	const Module = await ensureSimulideLoaded();
	return Module.ccall('getSimulationData', 'string', [], []);
}

export function activate(status?: 'onStartupFinished', arg?: string): void {
	console.log('Extension activated with status:', status, 'and arg:', arg);
	switch (status) {
		case 'onStartupFinished':
			eda.sch_Event.addSimulationEnginePullEventListener('sim-engine-monitor', 'all', async (eventType: SimEventType, props: any) => {
				switch (eventType) {
					case 'SESSION_START': {
						const payload = extractCircuitPayload(props);
						if (!payload.content) {
							debugLog('SESSION_START missing circuit content', props);
							break;
						}
						const result = await loadCircuitFromFile(payload);
						await startSimulation();
						debugLog('startSimulation done');
						break;
					}
					case 'SESSION_PAUSE':
						await pauseSimulation();
						break;
					case 'SESSION_RESUME':
						await resumeSimulation();
						break;
					case 'SESSION_STOP':
						await stopSimulation();
						break;
					case 'SPEED_SET': {
						const rawSpeed = Number(props?.speed ?? props?.value ?? 0);
						const speed = clampSpeed(rawSpeed);
						await setSimulationSpeed(speed);
						break;
					}
					case 'COMPONENT_UPDATE': {
						const update = extractComponentUpdate(props);
						const updateCirId = String(update.updateCirId ?? '');
						const attrInput = String(update.attrInput ?? '');
						const updateValue = String(update.updateValue ?? '');
						if (updateCirId && attrInput) {
							const ok = await updateCircuitData(updateCirId, attrInput, updateValue);
							debugLog('updateCircuitData result', ok);
						}
						break;
					}
				}
			});
			break;
	}
}

export async function readSimulationDataOnce(): Promise<void> {
	const json = await getSimulationData();
	debugLog('getSimulationData', json);
}
