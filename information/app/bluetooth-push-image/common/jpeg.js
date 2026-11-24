import { BLE_IMG_MAX } from './proto.js'

export const CANVAS_MAX = 1280

function tryChoose(sourceType) {
	return new Promise((resolve, reject) => {
		uni.chooseImage({
			count: 1,
			sourceType,
			success: (res) => resolve(res.tempFilePaths[0]),
			fail: reject
		})
	})
}

function galleryPick() {
	return new Promise((resolve, reject) => {
		if (typeof plus === 'undefined' || !plus.gallery) {
			reject(new Error('无法打开相册'))
			return
		}
		plus.gallery.pick((p) => resolve(p), reject, {
			filter: 'image',
			multiple: false,
			system: true
		})
	})
}

export function pickImage() {
	return tryChoose(['album', 'camera'])
		.catch(() => tryChoose(['album']))
		.catch(() => galleryPick())
}

export function getImageInfo(src) {
	return new Promise((resolve, reject) => {
		uni.getImageInfo({
			src,
			success: resolve,
			fail: reject
		})
	})
}

export function coverRect(sw, sh, tw, th) {
	const srcRatio = sw / sh
	const dstRatio = tw / th
	let sx = 0
	let sy = 0
	let cw = sw
	let ch = sh
	if (srcRatio > dstRatio) {
		ch = sh
		cw = sh * dstRatio
		sx = (sw - cw) / 2
		sy = 0
	} else {
		cw = sw
		ch = sw / dstRatio
		sx = 0
		sy = (sh - ch) / 2
	}
	return { sx, sy, sw: cw, sh: ch, dx: 0, dy: 0, dw: tw, dh: th }
}

export function containRect(sw, sh, tw, th) {
	const s = Math.min(tw / sw, th / sh)
	const dw = sw * s
	const dh = sh * s
	return {
		sx: 0,
		sy: 0,
		sw,
		sh,
		dx: (tw - dw) / 2,
		dy: (th - dh) / 2,
		dw,
		dh
	}
}

function plusUrlCandidates(p) {
	const s = String(p || '')
	const list = []
	const add = (x) => {
		if (x && list.indexOf(x) < 0) {
			list.push(x)
		}
	}
	add(s)
	if (s.indexOf('file://') === 0) {
		add(s.slice(7))
	} else if (s.indexOf('/') === 0) {
		add('file://' + s)
	}
	if (typeof plus !== 'undefined' && plus.io && plus.io.convertLocalFileSystemURL) {
		try {
			add(plus.io.convertLocalFileSystemURL(s))
		} catch (e) {
			/* ignore */
		}
	}
	return list
}

function dataUrlToU8(dataUrl) {
	const i = String(dataUrl || '').indexOf(',')
	const b64 = i >= 0 ? dataUrl.slice(i + 1) : dataUrl
	const bin = atob(b64)
	const u8 = new Uint8Array(bin.length)
	for (let j = 0; j < bin.length; j++) {
		u8[j] = bin.charCodeAt(j)
	}
	return u8
}

function readViaPlus(path) {
	return new Promise((resolve, reject) => {
		if (typeof plus === 'undefined' || !plus.io) {
			reject(new Error('plus.io 不可用'))
			return
		}
		plus.io.resolveLocalFileSystemURL(path, (entry) => {
			entry.file((file) => {
				const reader = new plus.io.FileReader()
				let finished = false
				const done = (e) => {
					if (finished) {
						return
					}
					finished = true
					try {
						const r = (e && e.target && e.target.result) || reader.result
						if (typeof r === 'string' && r.indexOf('data:') === 0) {
							resolve(dataUrlToU8(r))
							return
						}
						reject(new Error('FileReader 结果无法解析'))
					} catch (err) {
						reject(err)
					}
				}
				reader.onloadend = done
				reader.onerror = (err) => {
					if (finished) {
						return
					}
					finished = true
					reject(err)
				}
				reader.readAsDataURL(file)
			}, reject)
		}, reject)
	})
}

function readViaFsManager(path) {
	return new Promise((resolve, reject) => {
		if (typeof uni.getFileSystemManager !== 'function') {
			reject(new Error('no fs manager'))
			return
		}
		uni.getFileSystemManager().readFile({
			filePath: path,
			success: (res) => {
				if (res.data instanceof ArrayBuffer) {
					resolve(new Uint8Array(res.data))
					return
				}
				reject(new Error('readFile 返回类型不是 ArrayBuffer'))
			},
			fail: reject
		})
	})
}

export async function readFileU8(path) {
	const candidates = plusUrlCandidates(path)
	let last = null
	for (const p of candidates) {
		try {
			return await readViaPlus(p)
		} catch (e) {
			last = e
		}
		try {
			return await readViaFsManager(p)
		} catch (e) {
			last = e
		}
	}
	throw last || new Error('读文件失败')
}

export function canvasToJpeg(vm, canvasId, src, layout, tw, th, quality) {
	return new Promise((resolve, reject) => {
		const ctx = uni.createCanvasContext(canvasId, vm)
		ctx.clearRect(0, 0, CANVAS_MAX, CANVAS_MAX)
		ctx.setFillStyle('#000000')
		ctx.fillRect(0, 0, CANVAS_MAX, CANVAS_MAX)
		ctx.fillRect(0, 0, tw, th)
		ctx.drawImage(src, layout.sx, layout.sy, layout.sw, layout.sh, layout.dx, layout.dy, layout.dw, layout.dh)
		ctx.draw(true, () => {
			setTimeout(() => {
				uni.canvasToTempFilePath({
					canvasId,
					x: 0,
					y: 0,
					width: tw,
					height: th,
					destWidth: tw,
					destHeight: th,
					fileType: 'jpg',
					quality,
					success: (r) => resolve(r.tempFilePath),
					fail: reject
				}, vm)
			}, 80)
		})
	})
}

export async function encodeOnCanvas(vm, canvasId, src, mode, tw, th, info) {
	const layout = mode === 'crop'
		? coverRect(info.width, info.height, tw, th)
		: containRect(info.width, info.height, tw, th)
	const qualities = [0.8, 0.75, 0.7, 0.65, 0.6, 0.5, 0.4]
	for (const q of qualities) {
		const outPath = await canvasToJpeg(vm, canvasId, src, layout, tw, th, q)
		const u8 = await readFileU8(outPath)
		if (u8.length < 2 || u8[0] !== 0xFF || u8[1] !== 0xD8) {
			throw new Error('压缩结果不是 JPEG（缺 FF D8）')
		}
		if (u8.length <= BLE_IMG_MAX) {
			return { u8, quality: Math.round(q * 100), preview: outPath }
		}
	}
	throw new Error('quality=40 仍超过 96KB，请更换图片或减小输出尺寸')
}
