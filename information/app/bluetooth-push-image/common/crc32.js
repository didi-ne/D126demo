const TABLE = new Uint32Array(256)
for (let i = 0; i < 256; i++) {
	let c = i
	for (let j = 0; j < 8; j++) {
		c = (c >>> 1) ^ (c & 1 ? 0xEDB88320 : 0)
	}
	TABLE[i] = c >>> 0
}

export function crc32IsoHdlc(bytes) {
	let crc = 0xFFFFFFFF
	for (let i = 0; i < bytes.length; i++) {
		crc = TABLE[(crc ^ bytes[i]) & 0xFF] ^ (crc >>> 8)
	}
	return (crc ^ 0xFFFFFFFF) >>> 0
}
