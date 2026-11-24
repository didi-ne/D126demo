export const ADV_NAME = 'XMZX-D126'

export const UUID_SERVICE = '0000FF00-0000-1000-8000-00805F9B34FB'
export const UUID_CTRL = '0000FF01-0000-1000-8000-00805F9B34FB'
export const UUID_DATA = '0000FF02-0000-1000-8000-00805F9B34FB'
export const UUID_STATUS = '0000FF03-0000-1000-8000-00805F9B34FB'

export const BLE_IMG_MTU = 247
export const BLE_IMG_CHUNK_PAYLOAD = 232
export const BLE_IMG_DATA_PKT = 9 + BLE_IMG_CHUNK_PAYLOAD
export const BLE_IMG_WIN = 4
export const BLE_IMG_MAX = 98304
export const BLE_IMG_W = 360
export const BLE_IMG_H = 360
export const BLE_IMG_FMT_JPEG = 0

export const FLAG_START = 0x01
export const FLAG_MIDDLE = 0x02
export const FLAG_END = 0x03

export const ST = {
	OK: 0x00,
	BUSY: 0x01,
	RX: 0x02,
	CRC_PKT: 0x10,
	CRC_FILE: 0x11,
	SIZE_ERR: 0x12,
	SEQ_ERR: 0x13,
	FLAG_ERR: 0x14,
	DONE: 0x20
}

export const ST_NAME = {
	0x00: 'OK',
	0x01: 'BUSY',
	0x02: 'RX',
	0x10: 'CRC_PKT',
	0x11: 'CRC_FILE',
	0x12: 'SIZE_ERR',
	0x13: 'SEQ_ERR',
	0x14: 'FLAG_ERR',
	0x20: 'DONE'
}

export const TIMEOUT_OK_MS = 2000
export const TIMEOUT_RX_MS = 2000
export const TIMEOUT_DONE_MS = 5000
export const MAX_RETRY = 3

export function sameUuid(a, b) {
	const norm = (u) => String(u || '').replace(/-/g, '').toUpperCase()
	const expand = (n) => {
		if (n.length === 4) {
			return '0000' + n + '00001000800000805F9B34FB'
		}
		if (n.length === 8) {
			return n + '00001000800000805F9B34FB'
		}
		return n
	}
	return expand(norm(a)) === expand(norm(b))
}

export const SIZE_PRESETS = [
	{ w: 360, h: 360, label: '360×360' },
	{ w: 320, h: 320, label: '320×320' },
	{ w: 240, h: 240, label: '240×240' },
	{ w: 480, h: 480, label: '480×480' },
	{ w: 128, h: 128, label: '128×128' }
]

export function chunkCntOf(totalLen) {
	return Math.ceil(totalLen / BLE_IMG_CHUNK_PAYLOAD)
}

export function uuidShort(u) {
	const n = String(u || '').replace(/-/g, '').toUpperCase()
	return n.length >= 4 ? n.slice(-4) : n
}

export function charPropsText(p) {
	const x = p || {}
	const a = []
	if (x.read) {
		a.push('R')
	}
	if (x.write) {
		a.push('W')
	}
	if (x.writeNoResponse) {
		a.push('WNR')
	}
	if (x.notify) {
		a.push('N')
	}
	if (x.indicate) {
		a.push('I')
	}
	return a.length ? a.join(' ') : '-'
}

export function gattName(uuid) {
	if (sameUuid(uuid, UUID_SERVICE)) {
		return '推图服务'
	}
	if (sameUuid(uuid, UUID_CTRL)) {
		return 'CTRL  写 START'
	}
	if (sameUuid(uuid, UUID_DATA)) {
		return 'DATA  写 JPEG'
	}
	if (sameUuid(uuid, UUID_STATUS)) {
		return 'STATUS  Notify'
	}
	return uuidShort(uuid)
}

export function deviceLabel(d) {
	return (d && (d.localName || d.name)) || '未知设备'
}

export function deviceKind(name) {
	const n = String(name || '').toLowerCase()
	if (/airpods\s*max|头戴|over[\s-]?ear|headphone|headset|\bwh-|\bxm[345]\b|solo\s?[23]|quietcomfort|crusher|\bbh\d/.test(n) && !/buds|earbud|tws|earfun|openfit|shokz/.test(n)) {
		return 'overear'
	}
	if (/buds|earbud|tws|airpods|freebuds|enco|wf-|in[\s-]?ear|入耳|linkbuds|pixel buds|galaxy buds|redmi buds|bowie|soundpeats|qcy|edifier|漫步者|耳机|earfun|openfit|shokz|air\s*pro/.test(n)) {
		return 'earbuds'
	}
	if (/watch|band|手环|手表|\bgt\s*\d|\bpace\b|coros|garmin|amazfit/.test(n)) {
		return 'watch'
	}
	if (/speaker|音箱|soundcore|flip\s*\d/.test(n)) {
		return 'speaker'
	}
	return 'ble'
}

export function isBtOffErr(e) {
	if (!e) {
		return false
	}
	if (e.btOff) {
		return true
	}
	const code = e.errCode
	const raw = String(e.message || e.errMsg || e)
	return code === 10001 || /not available|蓝牙未开启|蓝牙不可用/i.test(raw)
}

export function isSysOccupiedErr(e) {
	return !!(e && e.sysOccupied)
}

export function bleErrText(e) {
	if (!e) {
		return 'unknown'
	}
	if (typeof e === 'string') {
		return e
	}
	if (e.btOff) {
		return '蓝牙未开启'
	}
	if (e.sysOccupied) {
		return '该设备已由系统蓝牙连接'
	}
	const code = e.errCode
	const raw = e.message || e.errMsg || JSON.stringify(e)
	const map = {
		10000: '蓝牙适配器未初始化，请先打开手机蓝牙后再扫描',
		10001: '蓝牙未开启',
		10002: '未找到设备，请靠近设备或确认其正在广播',
		10003: 'GATT 连接失败。设备可能未在广播，或仍被系统蓝牙占用',
		10004: '对端未提供 GATT 服务',
		10012: '连接超时',
		10013: 'deviceId 无效'
	}
	if (map[code]) {
		return map[code]
	}
	if (/already\s*connect|已经连接|已连接/i.test(raw)) {
		return 'already-connected'
	}
	if (/not init/i.test(raw)) {
		return '蓝牙适配器未初始化，请先打开手机蓝牙后再扫描'
	}
	if (/operate time\s*out|连接设备 超时/i.test(raw)) {
		return 'GATT 连接超时。系统刚断开后设备会暂停广播，请先点开始扫描，出现在可用设备后再连接'
	}
	return raw
}
