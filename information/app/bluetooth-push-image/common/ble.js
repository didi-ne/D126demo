import {
	UUID_SERVICE,
	UUID_CTRL,
	UUID_DATA,
	UUID_STATUS,
	BLE_IMG_MTU,
	BLE_IMG_DATA_PKT,
	sameUuid,
	deviceLabel,
	bleErrText,
	isBtOffErr
} from './proto.js'
import { parseStatus, toArrayBuffer } from './packet.js'

const HIST_KEY = 'ble_img_hist'

function promisify(fn, params) {
	return new Promise((resolve, reject) => {
		fn({
			...params,
			success: resolve,
			fail: reject
		})
	})
}

function sleep(ms) {
	return new Promise((r) => setTimeout(r, ms))
}

export function isConnectAbort(e) {
	return !!(e && (e.name === 'ConnectAbort' || (e.message || '') === '已取消连接'))
}

function withTimeout(p, ms, label) {
	let timer = null
	const timeout = new Promise((_, reject) => {
		timer = setTimeout(() => reject(new Error(label + ' 超时 ' + ms + 'ms')), ms)
	})
	return Promise.race([p, timeout]).then((v) => {
		clearTimeout(timer)
		return v
	}, (e) => {
		clearTimeout(timer)
		throw e
	})
}

function callTimeout(fn, params, ms, label) {
	return withTimeout(promisify(fn, params), ms, label)
}

export function loadHist() {
	try {
		const list = uni.getStorageSync(HIST_KEY)
		return Array.isArray(list) ? list : []
	} catch (e) {
		return []
	}
}

export function saveHist(device) {
	const item = {
		deviceId: device.deviceId,
		name: deviceLabel(device),
		ts: Date.now()
	}
	const list = loadHist().filter((x) => x.deviceId !== item.deviceId)
	list.unshift(item)
	uni.setStorageSync(HIST_KEY, list.slice(0, 20))
}

export function forgetHist(deviceId) {
	const list = loadHist().filter((x) => x.deviceId !== deviceId)
	uni.setStorageSync(HIST_KEY, list)
	return list
}

export class StatusWaiter {
	constructor() {
		this._queue = []
		this._pending = null
	}

	reset() {
		this._queue = []
		if (this._pending) {
			const { reject, timer } = this._pending
			clearTimeout(timer)
			this._pending = null
			reject(new Error('STATUS 等待被取消'))
		}
	}

	push(st) {
		if (this._pending) {
			const { resolve, timer } = this._pending
			this._pending = null
			clearTimeout(timer)
			resolve(st)
			return
		}
		this._queue.push(st)
	}

	poll() {
		if (this._queue.length) {
			return this._queue.shift()
		}
		return null
	}

	wait(ms, label) {
		if (this._queue.length) {
			return Promise.resolve(this._queue.shift())
		}
		return new Promise((resolve, reject) => {
			const timer = setTimeout(() => {
				this._pending = null
				reject(new Error(label + ' 超时 ' + ms + 'ms'))
			}, ms)
			this._pending = { resolve, reject, timer }
		})
	}
}

export class BleSession {
	constructor() {
		this.deviceId = ''
		this.deviceName = ''
		this.mtu = 23
		this.serviceId = UUID_SERVICE
		this.ctrlId = UUID_CTRL
		this.dataId = UUID_DATA
		this.statusId = UUID_STATUS
		this.connected = false
		this.ready = false
		this.protoReady = false
		this.scanning = false
		this.gattTree = []
		this.statusWaiter = new StatusWaiter()
		this._hooked = false
		this._onConn = null
		this._scanHandler = null
		this._onScanDevice = null
		this._onRx = null
		this._onAdapter = null
		this._onLog = null
		this._logs = []
		this._opened = false
		this._connectSeq = 0
	}

	_hookEvents() {
		if (this._hooked) {
			return
		}
		this._hooked = true
		uni.onBLEConnectionStateChange((res) => {
			if (res.deviceId !== this.deviceId) {
				return
			}
			this.connected = !!res.connected
			if (!res.connected) {
				this.ready = false
				this.protoReady = false
				this.statusWaiter.reset()
			}
			if (this._onConn) {
				this._onConn(this.connected)
			}
		})
		uni.onBLECharacteristicValueChange((res) => {
			const u8 = new Uint8Array(res.value || [])
			if (this._onRx) {
				this._onRx(res.characteristicId, u8)
			}
			if (!sameUuid(res.characteristicId, UUID_STATUS)) {
				return
			}
			try {
				this.statusWaiter.push(parseStatus(res.value))
			} catch (e) {
				console.error(e)
			}
		})
		uni.onBluetoothAdapterStateChange((res) => {
			const on = !!res.available
			if (!on) {
				this._opened = false
				this.scanning = false
			} else if (!this._opened) {
				promisify(uni.openBluetoothAdapter, {}).then(() => {
					this._opened = true
				}).catch((e) => {
					const raw = (e && (e.errMsg || e.message)) || ''
					if (/already/i.test(raw)) {
						this._opened = true
					}
				})
			}
			if (this._onAdapter) {
				this._onAdapter(on)
			}
		})
	}

	onRx(fn) {
		this._onRx = fn
	}

	onAdapterChange(fn) {
		this._onAdapter = fn
	}

	onConnectionChange(fn) {
		this._onConn = fn
	}

	onLog(fn) {
		this._onLog = fn
	}

	emitLog(msg) {
		const line = String(msg || '')
		this._logs.push(line)
		if (this._logs.length > 300) {
			this._logs = this._logs.slice(-300)
		}
		if (this._onLog) {
			this._onLog(line)
		}
	}

	takeLogs() {
		const list = this._logs.slice()
		this._logs = []
		return list
	}

	_btOffErr(e) {
		const err = new Error('蓝牙未开启')
		err.errCode = (e && e.errCode) || 10001
		err.btOff = true
		err.errMsg = (e && (e.errMsg || e.message)) || '蓝牙未开启'
		return err
	}

	openBtSettings() {
		try {
			if (typeof plus !== 'undefined' && plus.android) {
				const main = plus.android.runtimeMainActivity()
				const Intent = plus.android.importClass('android.content.Intent')
				const BluetoothAdapter = plus.android.importClass('android.bluetooth.BluetoothAdapter')
				const intent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE)
				main.startActivity(intent)
				return
			}
		} catch (e) {
			/* fall through */
		}
		uni.showToast({
			title: '请在系统控制中心打开蓝牙',
			icon: 'none'
		})
	}

	openBtPanel() {
		try {
			if (typeof plus !== 'undefined' && plus.android) {
				const main = plus.android.runtimeMainActivity()
				const Intent = plus.android.importClass('android.content.Intent')
				const Settings = plus.android.importClass('android.provider.Settings')
				main.startActivity(new Intent(Settings.ACTION_BLUETOOTH_SETTINGS))
				return
			}
		} catch (e) {
			/* fall through */
		}
		this.openBtSettings()
	}

	querySysLink(deviceId) {
		const mac = String(deviceId || '').toUpperCase()
		if (!mac || typeof plus === 'undefined' || !plus.android) {
			return []
		}
		const names = []
		const add = (n) => {
			if (n && names.indexOf(n) < 0) {
				names.push(n)
			}
		}
		try {
			const main = plus.android.runtimeMainActivity()
			const BluetoothManager = plus.android.importClass('android.bluetooth.BluetoothManager')
			const BluetoothProfile = plus.android.importClass('android.bluetooth.BluetoothProfile')
			const bm = plus.android.cast(BluetoothManager, main.getSystemService('bluetooth'))
			const adapter = bm.getAdapter()
			const dev = adapter.getRemoteDevice(mac)
			const STATE_CONNECTED = 2
			const profiles = [
				['A2DP', BluetoothProfile.A2DP],
				['HEADSET', BluetoothProfile.HEADSET],
				['GATT', BluetoothProfile.GATT]
			]
			try {
				profiles.push(['HID', BluetoothProfile.HID_HOST || 4])
			} catch (e) {
				profiles.push(['HID', 4])
			}
			profiles.push(['HEARING_AID', 21])
			profiles.push(['LE_AUDIO', 22])
			for (let i = 0; i < profiles.length; i++) {
				const n = profiles[i][0]
				const id = profiles[i][1]
				try {
					if (bm.getConnectionState(dev, id) === STATE_CONNECTED) {
						add(n)
					}
				} catch (e) {
					/* 该机型没有这个 profile */
				}
				try {
					const list = bm.getConnectedDevices(id)
					const size = list ? list.size() : 0
					for (let j = 0; j < size; j++) {
						const item = list.get(j)
						if (String(item.getAddress()).toUpperCase() === mac) {
							add(n)
						}
					}
				} catch (e2) {
					/* getConnectedDevices 在部分 profile 上会抛 */
				}
			}
		} catch (e) {
			return names
		}
		return names
	}

	_sysOccupiedErr(device, profiles) {
		const name = deviceLabel(device)
		const err = new Error('该设备已由系统蓝牙连接')
		err.sysOccupied = true
		err.profiles = profiles || []
		err.deviceName = name
		return err
	}

	async _seenInCache(deviceId) {
		const mac = String(deviceId || '').toUpperCase()
		try {
			const res = await promisify(uni.getBluetoothDevices, {})
			const list = res.devices || []
			return list.some((d) => String(d.deviceId || '').toUpperCase() === mac)
		} catch (e) {
			return false
		}
	}

	async ensureDiscovered(deviceId, seq) {
		this._checkConnect(seq)
		if (await this._seenInCache(deviceId)) {
			return
		}
		this.emitLog('等待设备广播: ' + deviceId)
		const mac = String(deviceId).toUpperCase()
		await new Promise((resolve, reject) => {
			let settled = false
			const finish = (err) => {
				if (settled) {
					return
				}
				settled = true
				this.stopScan().then(() => {
					if (err) {
						reject(err)
					} else {
						resolve()
					}
				}, () => {
					if (err) {
						reject(err)
					} else {
						resolve()
					}
				})
			}
			const timer = setTimeout(() => {
				const e = new Error('未扫描到该设备。系统蓝牙刚断开时设备会暂停广播，请先点开始扫描，出现在可用设备后再连接')
				e.needScan = true
				finish(e)
			}, 10000)
			this.startScan((d) => {
				if (String(d.deviceId || '').toUpperCase() === mac) {
					clearTimeout(timer)
					finish(null)
				}
			}).catch((e) => {
				clearTimeout(timer)
				finish(e)
			})
		})
		this._checkConnect(seq)
		await sleep(250)
		this._checkConnect(seq)
	}

	async ensureOpen() {
		this._hookEvents()
		await this.requestPerm()
		if (this._opened) {
			try {
				const st = await promisify(uni.getBluetoothAdapterState, {})
				if (st.available) {
					return
				}
			} catch (e) {
				/* 未初始化，下面重新 open */
			}
			this._opened = false
		}
		try {
			await promisify(uni.openBluetoothAdapter, {})
			this._opened = true
		} catch (e) {
			const raw = (e && (e.errMsg || e.message)) || ''
			if (/already/i.test(raw)) {
				this._opened = true
				return
			}
			this._opened = false
			if (isBtOffErr(e) || (e && e.errCode === 10001)) {
				throw this._btOffErr(e)
			}
			throw e
		}
	}

	async open() {
		return this.ensureOpen()
	}

	async requestPerm() {
		const sys = uni.getSystemInfoSync()
		if (sys.platform !== 'android') {
			return
		}
		if (typeof plus === 'undefined' || !plus.android || !plus.android.requestPermissions) {
			return
		}
		let sdk = 0
		try {
			const Build = plus.android.importClass('android.os.Build')
			sdk = Build.VERSION.SDK_INT
		} catch (e) {
			sdk = 0
		}
		const perms = ['android.permission.ACCESS_FINE_LOCATION']
		if (sdk >= 31) {
			perms.push('android.permission.BLUETOOTH_SCAN')
			perms.push('android.permission.BLUETOOTH_CONNECT')
		}
		if (sdk >= 33) {
			perms.push('android.permission.READ_MEDIA_IMAGES')
		} else {
			perms.push('android.permission.READ_EXTERNAL_STORAGE')
		}
		await new Promise((resolve) => {
			plus.android.requestPermissions(perms, () => resolve(), () => resolve())
		})
	}

	async startScan(onDevice) {
		await this.ensureOpen()
		await this.stopScan()
		this._onScanDevice = onDevice
		this._scanHandler = (res) => {
			const list = res.devices || []
			for (const d of list) {
				if (!d.deviceId) {
					continue
				}
				if (this._onScanDevice) {
					this._onScanDevice(d)
				}
			}
		}
		uni.onBluetoothDeviceFound(this._scanHandler)
		await callTimeout(uni.startBluetoothDevicesDiscovery, {
			allowDuplicatesKey: true,
			interval: 800
		}, 4000, '开始扫描')
		this.scanning = true
	}

	async stopScan() {
		this.scanning = false
		this._onScanDevice = null
		try {
			await callTimeout(uni.stopBluetoothDevicesDiscovery, {}, 1500, '停止扫描')
		} catch (e) {
			/* 未在扫描或停止无回调 */
		}
		if (this._scanHandler) {
			try {
				uni.offBluetoothDeviceFound(this._scanHandler)
			} catch (e) {
				/* 部分运行时没有 off */
			}
			this._scanHandler = null
		}
	}

	_checkConnect(seq) {
		if (seq !== this._connectSeq) {
			const e = new Error('已取消连接')
			e.name = 'ConnectAbort'
			throw e
		}
	}

	abortConnect() {
		this._connectSeq++
		this.ready = false
		this.protoReady = false
		this.connected = false
		this.statusWaiter.reset()
		const id = this.deviceId
		if (!id) {
			return
		}
		promisify(uni.closeBLEConnection, { deviceId: id }).catch(() => {})
	}

	async connect(device) {
		const seq = ++this._connectSeq
		await this.ensureOpen()
		this._checkConnect(seq)
		const occupied = this.querySysLink(device.deviceId)
		if (occupied.length) {
			this.emitLog('系统蓝牙占用: ' + occupied.join(','))
			throw this._sysOccupiedErr(device, occupied)
		}
		this.deviceId = device.deviceId
		this.deviceName = deviceLabel(device)
		this.statusWaiter.reset()
		this.ready = false
		this.protoReady = false
		this.mtu = 23
		this.gattTree = []
		this._logs = []
		await this.ensureDiscovered(device.deviceId, seq)
		this._checkConnect(seq)
		await this.stopScan()
		this._checkConnect(seq)
		await sleep(200)
		this._checkConnect(seq)
		try {
			await this._createConn()
		} catch (e) {
			this._checkConnect(seq)
			throw e
		}
		this._checkConnect(seq)
		this.connected = true
		this.emitLog('已连接: ' + this.deviceId)
		await sleep(300)
		this._checkConnect(seq)
		try {
			await withTimeout(this._discoverRetry(), 12000, '发现服务')
		} catch (e) {
			this._checkConnect(seq)
			this.protoReady = false
			this.emitLog('发现服务失败: ' + bleErrText(e))
		}
		this._checkConnect(seq)
		try {
			await withTimeout(this._requestMtu(), 4000, '协商 MTU')
		} catch (e) {
			this._checkConnect(seq)
			this.emitLog('MTU 协商失败: ' + bleErrText(e))
		}
		this._checkConnect(seq)
		if (this.protoReady) {
			try {
				await callTimeout(uni.notifyBLECharacteristicValueChange, {
					deviceId: this.deviceId,
					serviceId: this.serviceId,
					characteristicId: this.statusId,
					state: true
				}, 3000, '打开 STATUS 通知')
				this.emitLog('STATUS 通知已开启: ' + this.statusId)
			} catch (e) {
				this._checkConnect(seq)
				this.emitLog('STATUS 通知开启失败: ' + bleErrText(e))
			}
			this._checkConnect(seq)
			await sleep(200)
			this._checkConnect(seq)
			const payload = this.mtu - 3
			if (this.mtu >= 100 && payload >= BLE_IMG_DATA_PKT) {
				this.ready = true
				this.emitLog('推图服务已就绪 MTU=' + this.mtu)
			} else {
				this.emitLog('MTU=' + this.mtu + ' 不足以发送推图 DATA')
			}
		}
		this._checkConnect(seq)
	}

	async _createConn() {
		const once = (ms) => callTimeout(uni.createBLEConnection, {
			deviceId: this.deviceId,
			timeout: ms
		}, ms + 1000, '连接设备')
		try {
			await once(8000)
			return
		} catch (e) {
			if (bleErrText(e) === 'already-connected') {
				return
			}
			try {
				await callTimeout(uni.closeBLEConnection, {
					deviceId: this.deviceId
				}, 2000, '断开')
			} catch (e2) {
				/* 忽略 */
			}
			await sleep(400)
			try {
				await once(12000)
			} catch (e3) {
				if (bleErrText(e3) === 'already-connected') {
					return
				}
				throw new Error(bleErrText(e3))
			}
		}
	}

	async _discoverRetry() {
		let last = null
		for (let i = 0; i < 3; i++) {
			try {
				await this._discover()
				return
			} catch (e) {
				last = e
				await sleep(400)
			}
		}
		throw last || new Error('发现服务失败')
	}

	async _requestMtu() {
		const wanted = BLE_IMG_MTU
		const current = this.mtu || 23
		this.emitLog('请求MTU: wanted=' + wanted + ' 当前=' + current + ' try=1 api=setBLEMTU')
		let setOk = false
		try {
			const res = await callTimeout(uni.setBLEMTU, {
				deviceId: this.deviceId,
				mtu: wanted
			}, 3000, 'setBLEMTU')
			setOk = true
			this.emitLog('setBLEMTU success: ' + JSON.stringify(res || { message: 'ok' }))
		} catch (e) {
			this.emitLog('setBLEMTU 失败: ' + bleErrText(e))
		}
		let mtu = 0
		try {
			const res = await callTimeout(uni.getBLEMTU, {
				deviceId: this.deviceId
			}, 3000, 'getBLEMTU')
			if (typeof res === 'number') {
				mtu = res
			} else if (res && typeof res.mtu === 'number') {
				mtu = res.mtu
			} else {
				this.emitLog('getBLEMTU 回报: ' + JSON.stringify(res || {}))
			}
		} catch (e) {
			this.emitLog('getBLEMTU 失败: ' + bleErrText(e))
		}
		if (!mtu) {
			if (setOk) {
				mtu = wanted
				this.emitLog('MTU协商成功但回报缺少mtu，先按' + wanted + '尝试 ' + (wanted - 7) + '发送')
			} else {
				mtu = current
			}
		} else {
			this.emitLog('当前MTU=' + mtu + ' 分片=' + Math.max(mtu - 7, 0))
		}
		this.mtu = mtu
	}

	async _discover() {
		const svc = await callTimeout(uni.getBLEDeviceServices, {
			deviceId: this.deviceId
		}, 8000, '读取服务')
		const services = svc.services || []
		this.emitLog('服务数量: ' + services.length)
		const tree = []
		for (let i = 0; i < services.length; i++) {
			const s = services[i]
			let chars = []
			try {
				const ch = await callTimeout(uni.getBLEDeviceCharacteristics, {
					deviceId: this.deviceId,
					serviceId: s.uuid
				}, 4000, '读取特征')
				chars = (ch.characteristics || []).map((c) => ({
					uuid: c.uuid,
					properties: c.properties || {}
				}))
			} catch (e) {
				chars = []
			}
			this.emitLog('特征值数量[' + s.uuid + ']: ' + chars.length)
			tree.push({
				uuid: s.uuid,
				isPush: sameUuid(s.uuid, UUID_SERVICE),
				chars
			})
		}
		this.gattTree = tree
		this.protoReady = false
		const push = tree.find((s) => s.isPush)
		if (!push) {
			return
		}
		this.serviceId = push.uuid
		const ctrl = push.chars.find((c) => sameUuid(c.uuid, UUID_CTRL))
		const data = push.chars.find((c) => sameUuid(c.uuid, UUID_DATA))
		const status = push.chars.find((c) => sameUuid(c.uuid, UUID_STATUS))
		if (!ctrl || !data || !status) {
			return
		}
		this.ctrlId = ctrl.uuid
		this.dataId = data.uuid
		this.statusId = status.uuid
		this.protoReady = true
	}

	readChar(serviceId, charId) {
		return callTimeout(uni.readBLECharacteristicValue, {
			deviceId: this.deviceId,
			serviceId,
			characteristicId: charId
		}, 5000, '读取特征')
	}

	notifyChar(serviceId, charId, state) {
		return callTimeout(uni.notifyBLECharacteristicValueChange, {
			deviceId: this.deviceId,
			serviceId,
			characteristicId: charId,
			state: !!state
		}, 5000, '设置通知')
	}

	writeChar(serviceId, charId, u8, writeType) {
		return callTimeout(uni.writeBLECharacteristicValue, {
			deviceId: this.deviceId,
			serviceId,
			characteristicId: charId,
			value: toArrayBuffer(u8),
			writeType: writeType || 'writeNoResponse'
		}, 5000, '写入特征')
	}

	writeCtrl(u8) {
		return promisify(uni.writeBLECharacteristicValue, {
			deviceId: this.deviceId,
			serviceId: this.serviceId,
			characteristicId: this.ctrlId,
			value: toArrayBuffer(u8),
			writeType: 'write'
		})
	}

	async writeData(u8) {
		const params = {
			deviceId: this.deviceId,
			serviceId: this.serviceId,
			characteristicId: this.dataId,
			value: toArrayBuffer(u8)
		}
		const send = (writeType) => promisify(uni.writeBLECharacteristicValue, {
			...params,
			writeType
		})
		const propErr = (e) => {
			if (!e || typeof e !== 'object') {
				return false
			}
			const msg = e.errMsg || e.message || ''
			return String(msg).indexOf('property not support') >= 0 || e.errCode === 10007
		}
		const gaps = [0, 80, 160, 300, 500]
		let last = null
		for (let i = 0; i < gaps.length; i++) {
			if (gaps[i]) {
				await sleep(gaps[i])
			}
			try {
				await send('writeNoResponse')
				last = null
				break
			} catch (e) {
				last = e
				if (!propErr(e)) {
					throw e
				}
			}
		}
		if (last) {
			await sleep(200)
			try {
				await send('write')
			} catch (e) {
				if (!propErr(e)) {
					throw e
				}
				throw last
			}
		}
		await sleep(30)
	}

	writeRaw(u8, serviceId, charId, writeType) {
		const sid = serviceId || this.serviceId
		const cid = charId || this.dataId
		if (!sid || !cid) {
			throw new Error('未选择可写特征')
		}
		return this.writeChar(sid, cid, u8, writeType || 'writeNoResponse')
	}

	async disconnect() {
		this.ready = false
		this.protoReady = false
		this.statusWaiter.reset()
		if (!this.deviceId) {
			this.connected = false
			return
		}
		try {
			await promisify(uni.closeBLEConnection, {
				deviceId: this.deviceId
			})
		} catch (e) {
			/* 已断开 */
		}
		this.connected = false
	}
}
