import { crc32IsoHdlc } from './crc32.js'
import {
	FLAG_START,
	FLAG_MIDDLE,
	FLAG_END,
	BLE_IMG_CHUNK_PAYLOAD,
	BLE_IMG_FMT_JPEG,
	chunkCntOf
} from './proto.js'

export function toArrayBuffer(u8) {
	return u8.buffer.slice(u8.byteOffset, u8.byteOffset + u8.byteLength)
}

export function buildStart(jpegU8, width, height) {
	const total = jpegU8.length
	const chunks = chunkCntOf(total)
	const buf = new Uint8Array(26)
	const dv = new DataView(buf.buffer)
	buf[0] = FLAG_START
	dv.setUint16(1, 0, true)
	buf[3] = 0x49
	buf[4] = 0x4D
	buf[5] = 0x47
	buf[6] = 0x50
	dv.setUint32(7, total, true)
	dv.setUint16(11, width, true)
	dv.setUint16(13, height, true)
	buf[15] = BLE_IMG_FMT_JPEG
	dv.setUint16(16, chunks, true)
	dv.setUint32(18, crc32IsoHdlc(jpegU8), true)
	dv.setUint32(22, crc32IsoHdlc(buf.subarray(0, 22)), true)
	return buf
}

export function buildData(jpegU8, seq, chunkCnt) {
	const offset = (seq - 1) * BLE_IMG_CHUNK_PAYLOAD
	const plen = Math.min(BLE_IMG_CHUNK_PAYLOAD, jpegU8.length - offset)
	const buf = new Uint8Array(9 + plen)
	const dv = new DataView(buf.buffer)
	buf[0] = seq === chunkCnt ? FLAG_END : FLAG_MIDDLE
	dv.setUint16(1, seq, true)
	dv.setUint16(3, plen, true)
	buf.set(jpegU8.subarray(offset, offset + plen), 5)
	dv.setUint32(5 + plen, crc32IsoHdlc(buf.subarray(0, 5 + plen)), true)
	return buf
}

export function parseStatus(ab) {
	const u8 = new Uint8Array(ab)
	if (u8.length < 5) {
		throw new Error('STATUS 长度 < 5')
	}
	const dv = new DataView(u8.buffer, u8.byteOffset, u8.byteLength)
	return {
		code: u8[0],
		value: dv.getUint32(1, true)
	}
}

export function parseHex(s) {
	const t = String(s || '').replace(/0x/gi, '').replace(/[\s,]/g, '')
	if (!t.length) {
		throw new Error('发送内容为空')
	}
	if (t.length % 2) {
		throw new Error('HEX 长度必须为偶数')
	}
	if (!/^[0-9a-fA-F]+$/.test(t)) {
		throw new Error('HEX 包含无效字符')
	}
	const u8 = new Uint8Array(t.length / 2)
	for (let i = 0; i < u8.length; i++) {
		u8[i] = parseInt(t.substr(i * 2, 2), 16)
	}
	return u8
}

export function textToU8(s) {
	const str = String(s || '')
	if (!str.length) {
		throw new Error('发送内容为空')
	}
	if (typeof TextEncoder !== 'undefined') {
		return new TextEncoder().encode(str)
	}
	const u8 = new Uint8Array(str.length)
	for (let i = 0; i < str.length; i++) {
		u8[i] = str.charCodeAt(i) & 0xFF
	}
	return u8
}

export function asciiToU8(s) {
	const str = String(s || '')
	if (!str.length) {
		throw new Error('发送内容为空')
	}
	const u8 = new Uint8Array(str.length)
	for (let i = 0; i < str.length; i++) {
		const c = str.charCodeAt(i)
		if (c > 127) {
			throw new Error('ASCII 仅支持 0–127')
		}
		u8[i] = c
	}
	return u8
}

export function base64ToU8(s) {
	const t = String(s || '').replace(/\s/g, '')
	if (!t.length) {
		throw new Error('发送内容为空')
	}
	let bin = ''
	try {
		bin = atob(t)
	} catch (e) {
		throw new Error('Base64 解码失败')
	}
	const u8 = new Uint8Array(bin.length)
	for (let i = 0; i < bin.length; i++) {
		u8[i] = bin.charCodeAt(i)
	}
	return u8
}

export function encodeTx(s, mode) {
	if (mode === 'HEX') {
		return parseHex(s)
	}
	if (mode === 'ASCII') {
		return asciiToU8(s)
	}
	if (mode === 'Base64') {
		return base64ToU8(s)
	}
	return textToU8(s)
}

export function bytesToHex(u8) {
	const arr = u8 instanceof Uint8Array ? u8 : new Uint8Array(u8 || [])
	return Array.from(arr).map((b) => ('0' + b.toString(16)).slice(-2)).join('')
}

export function formatRx(u8, mode) {
	const arr = u8 instanceof Uint8Array ? u8 : new Uint8Array(u8 || [])
	if (!arr.length) {
		return ''
	}
	if (mode === 'HEX') {
		return Array.from(arr).map((b) => ('0' + b.toString(16)).slice(-2).toUpperCase()).join('\n')
	}
	if (mode === 'ASCII') {
		return Array.from(arr).map((b) => (b >= 32 && b < 127) ? String.fromCharCode(b) : '.').join('')
	}
	if (mode === 'UTF-8') {
		try {
			if (typeof TextDecoder !== 'undefined') {
				return new TextDecoder('utf-8').decode(arr)
			}
		} catch (e) {
			return '[UTF-8 解码失败]'
		}
		return Array.from(arr).map((b) => String.fromCharCode(b)).join('')
	}
	let bin = ''
	for (let i = 0; i < arr.length; i++) {
		bin += String.fromCharCode(arr[i])
	}
	try {
		return btoa(bin)
	} catch (e) {
		return '[Base64 失败]'
	}
}
