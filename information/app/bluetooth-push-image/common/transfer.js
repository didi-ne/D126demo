import {
	ST,
	ST_NAME,
	BLE_IMG_WIN,
	BLE_IMG_MAX,
	MAX_RETRY,
	TIMEOUT_OK_MS,
	TIMEOUT_RX_MS,
	TIMEOUT_DONE_MS,
	chunkCntOf
} from './proto.js'
import { buildStart, buildData } from './packet.js'

function sleep(ms) {
	return new Promise((r) => setTimeout(r, ms))
}

function isHardErr(code) {
	return code === ST.CRC_PKT || code === ST.CRC_FILE ||
		code === ST.SIZE_ERR || code === ST.SEQ_ERR || code === ST.FLAG_ERR
}

function fmtSt(st) {
	const name = ST_NAME[st.code] || ('UNK_0x' + st.code.toString(16))
	return name + ' value=' + st.value
}

export function isAbortErr(e) {
	const msg = (e && e.message) || ''
	return !!(e && (e.name === 'AbortErr' || msg === '已停止发送' || msg === 'STATUS 等待被取消'))
}

function throwIfAbort(abort) {
	if (abort && abort.aborted) {
		const e = new Error('已停止发送')
		e.name = 'AbortErr'
		throw e
	}
}

export async function pushJpeg(session, jpegU8, width, height, onLog, onProgress, abort) {
	if (!jpegU8 || jpegU8.length < 1 || jpegU8.length > BLE_IMG_MAX) {
		throw new Error('JPEG 长度非法: ' + (jpegU8 ? jpegU8.length : 0))
	}
	const chunks = chunkCntOf(jpegU8.length)
	let lastErr = null
	for (let attempt = 1; attempt <= MAX_RETRY; attempt++) {
		throwIfAbort(abort)
		onLog && onLog('第 ' + attempt + ' 次，' + width + 'x' + height + ' chunk_cnt=' + chunks + ' len=' + jpegU8.length)
		try {
			await pushOnce(session, jpegU8, width, height, chunks, onLog, onProgress, abort)
			return
		} catch (e) {
			lastErr = e
			if (isAbortErr(e)) {
				onLog && onLog('已停止发送')
				throw e
			}
			onLog && onLog('失败: ' + ((e && (e.errMsg || e.message)) || e))
			if (!session.ready) {
				throw e
			}
		}
	}
	throw lastErr || new Error('推图失败')
}

async function pushOnce(session, jpegU8, width, height, chunks, onLog, onProgress, abort) {
	throwIfAbort(abort)
	session.statusWaiter.reset()
	await session.writeCtrl(buildStart(jpegU8, width, height))
	throwIfAbort(abort)
	onLog && onLog('已发 START，等 OK')
	let st = await session.statusWaiter.wait(TIMEOUT_OK_MS, '等 OK')
	throwIfAbort(abort)
	onLog && onLog('STATUS ' + fmtSt(st))
	if (st.code === ST.BUSY) {
		await sleep(1000)
		throw new Error('BUSY')
	}
	if (st.code !== ST.OK) {
		throw new Error('START 后非 OK: ' + fmtSt(st))
	}
	const win = (st.value >= 1 && st.value <= 8) ? st.value : BLE_IMG_WIN
	onLog && onLog('窗口 ' + win)

	let seq = 1
	while (seq <= chunks) {
		throwIfAbort(abort)
		const winEnd = Math.min(seq + win - 1, chunks)
		for (let s = seq; s <= winEnd; s++) {
			throwIfAbort(abort)
			await session.writeData(buildData(jpegU8, s, chunks))
			onProgress && onProgress({ seq: s, chunkCnt: chunks })
		}
		if (winEnd === chunks) {
			throwIfAbort(abort)
			onLog && onLog('已发 END seq=' + winEnd + '，等 DONE')
			st = await session.statusWaiter.wait(TIMEOUT_DONE_MS, '等 DONE')
			throwIfAbort(abort)
			onLog && onLog('STATUS ' + fmtSt(st))
			if (st.code === ST.DONE) {
				return
			}
			throw new Error('END 后失败: ' + fmtSt(st))
		}
		let guard = 0
		let from = seq
		while (true) {
			throwIfAbort(abort)
			onLog && onLog('窗 ' + from + '..' + winEnd + ' 等 RX')
			st = await session.statusWaiter.wait(TIMEOUT_RX_MS, '等 RX')
			throwIfAbort(abort)
			let best = st
			let extra = session.statusWaiter.poll()
			while (extra) {
				if (extra.code === ST.RX && best.code === ST.RX && extra.value > best.value) {
					best = extra
				} else if (isHardErr(extra.code)) {
					best = extra
				}
				extra = session.statusWaiter.poll()
			}
			if (best.code === ST.RX && best.value !== winEnd + 1) {
				await sleep(80)
				extra = session.statusWaiter.poll()
				while (extra) {
					if (extra.code === ST.RX && extra.value > best.value) {
						best = extra
					} else if (isHardErr(extra.code)) {
						best = extra
					}
					extra = session.statusWaiter.poll()
				}
			}
			st = best
			onLog && onLog('STATUS ' + fmtSt(st))
			if (isHardErr(st.code)) {
				throw new Error(fmtSt(st))
			}
			if (st.code !== ST.RX) {
				throw new Error('期望 RX: ' + fmtSt(st))
			}
			if (st.value === winEnd + 1) {
				seq = st.value
				break
			}
			if (st.value <= from || st.value > winEnd) {
				throw new Error('RX.value=' + st.value + ' 期望 ' + (winEnd + 1))
			}
			onLog && onLog('续传 seq=' + st.value)
			for (let s = st.value; s <= winEnd; s++) {
				throwIfAbort(abort)
				await session.writeData(buildData(jpegU8, s, chunks))
				onProgress && onProgress({ seq: s, chunkCnt: chunks })
			}
			from = st.value
			guard++
			if (guard > 4) {
				throw new Error('续传失败 seq=' + st.value)
			}
		}
	}
}
