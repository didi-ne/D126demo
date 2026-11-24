<template>
	<view class="page">
		<button class="pick" :disabled="busy" @click="onPick">选择图片</button>

		<view class="modes">
			<view class="chip" :class="{ on: mode === 'crop' }" @click="onMode('crop')">铺满裁剪</view>
			<view class="chip" :class="{ on: mode === 'scale' }" @click="onMode('scale')">等比居中</view>
		</view>

		<view v-if="srcPath" class="sizes">
			<view
				class="chip"
				v-for="s in presets"
				:key="s.label"
				:class="{ on: outW === s.w && outH === s.h }"
				@click="onSize(s.w, s.h)"
			>
				<text>{{ s.label }}</text>
			</view>
			<view class="chip" :class="{ on: customOn }" @click="customOn = !customOn">
				<text>自定义</text>
			</view>
		</view>
		<view v-if="customOn" class="custom">
			<input class="inp" type="number" v-model="customW" placeholder="宽" />
			<text class="x">×</text>
			<input class="inp" type="number" v-model="customH" placeholder="高" />
			<button class="ok" size="mini" type="primary" :disabled="busy" @click="onCustomSize">确定</button>
		</view>

		<view class="block" @click="onPreview(srcPath)">
			<text class="tag">原图</text>
			<image v-if="srcPath" class="pic" :src="srcPath" mode="widthFix"></image>
			<text v-else class="ph">选择图片后显示，点击可放大</text>
		</view>

		<view class="block" @click="onPreview(dstPath)">
			<text class="tag tag-r">{{ rightTag }}</text>
			<image v-if="dstPath" class="pic" :src="dstPath" :key="dstPath" mode="widthFix"></image>
			<text v-else class="ph">处理后显示，点击可放大</text>
		</view>

		<view v-if="chunkCnt" class="prog">
			<progress :percent="percent" stroke-width="6" activeColor="#007aff"></progress>
			<text class="prog-txt">{{ sentSeq }} / {{ chunkCnt }}</text>
		</view>

		<view class="log-card">
			<text class="sec">日志</text>
			<scroll-view scroll-y class="log">
				<text class="log-line" v-for="(line, i) in logs" :key="i">{{ line }}</text>
			</scroll-view>
		</view>

		<text v-if="!ready" class="warn">当前设备未提供 FF00 推图服务，暂无法发送图片</text>
		<button class="send" type="primary" :disabled="busy" @click="onPush">发送</button>

		<canvas
			canvas-id="work"
			id="work"
			class="work"
			:style="{ width: canvasMax + 'px', height: canvasMax + 'px' }"
			:width="canvasMax"
			:height="canvasMax"
		></canvas>

		<view v-if="sending" class="mask">
			<view class="dlg">
				<text class="dlg-t">发送中</text>
				<view class="dlg-bar">
					<progress class="dlg-p" :percent="percent" stroke-width="6" activeColor="#007aff"></progress>
					<text class="dlg-pct">{{ percent }}%</text>
				</view>
				<text class="dlg-sub">{{ sentSeq }} / {{ chunkCnt || '—' }}</text>
				<button class="dlg-stop" :disabled="stopping" @click="onStop">停止发送</button>
			</view>
		</view>
	</view>
</template>

<script>
	import { ble } from '../../common/session.js'
	import { SIZE_PRESETS } from '../../common/proto.js'
	import { pickImage, getImageInfo, encodeOnCanvas, CANVAS_MAX } from '../../common/jpeg.js'
	import { pushJpeg, isAbortErr } from '../../common/transfer.js'

	export default {
		data() {
			return {
				busy: false,
				ready: false,
				srcPath: '',
				dstPath: '',
				mode: 'crop',
				customOn: false,
				customW: '360',
				customH: '360',
				outW: 360,
				outH: 360,
				presets: SIZE_PRESETS,
				hasJpeg: false,
				sentSeq: 0,
				chunkCnt: 0,
				logs: [],
				sending: false,
				stopping: false,
				canvasMax: CANVAS_MAX
			}
		},
		computed: {
			rightTag() {
				const name = this.mode === 'crop' ? '铺满裁剪' : '等比居中'
				if (!this.dstPath) {
					return name
				}
				return name + '  ' + this.outW + '×' + this.outH
			},
			percent() {
				if (!this.chunkCnt) {
					return 0
				}
				return Math.min(100, Math.round(this.sentSeq * 100 / this.chunkCnt))
			}
		},
		onLoad() {
			this._jpegU8 = null
			this._origPath = ''
			this._origInfo = null
			this._applyToken = 0
			this._abort = { aborted: false }
			this.ready = ble.ready
			if (!ble.connected) {
				uni.showToast({
					title: '设备未连接',
					icon: 'none'
				})
				uni.navigateBack()
			}
		},
		onUnload() {
			if (this.sending) {
				this.onStop()
			}
		},
		methods: {
			log(msg) {
				const d = new Date()
				const pad = (n) => (n < 10 ? '0' + n : '' + n)
				const t = pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds())
				this.logs = [(t + '  ' + msg)].concat(this.logs).slice(0, 40)
			},
			errText(e) {
				if (!e) {
					return 'unknown'
				}
				if (typeof e === 'string') {
					return e
				}
				return e.message || e.errMsg || JSON.stringify(e)
			},
			onPreview(src) {
				if (!src) {
					return
				}
				const urls = []
				if (this.srcPath) {
					urls.push(this.srcPath)
				}
				if (this.dstPath && this.dstPath !== this.srcPath) {
					urls.push(this.dstPath)
				}
				uni.previewImage({
					current: src,
					urls: urls.length ? urls : [src]
				})
			},
			async onPick() {
				this.busy = true
				try {
					const src = await pickImage()
					const info = await getImageInfo(src)
					this.srcPath = src
					this._origPath = src
					this._origInfo = {
						width: info.width,
						height: info.height
					}
					this.dstPath = ''
					this._jpegU8 = null
					this.hasJpeg = false
					this.log('已选图 ' + info.width + '×' + info.height)
					if (this.mode) {
						await this.applySize(this.outW, this.outH)
					}
				} catch (e) {
					this.log('选图失败: ' + this.errText(e))
				} finally {
					this.busy = false
				}
			},
			async onMode(mode) {
				this.mode = mode
				if (this.srcPath) {
					await this.applySize(this.outW, this.outH)
				}
			},
			async onSize(w, h) {
				this.customOn = false
				await this.applySize(w, h)
			},
			async onCustomSize() {
				const w = parseInt(this.customW, 10)
				const h = parseInt(this.customH, 10)
				if (!w || !h || w < 8 || h < 8 || w > 1280 || h > 1280) {
					uni.showToast({
						title: '尺寸无效',
						icon: 'none'
					})
					return
				}
				await this.applySize(w, h)
			},
			async applySize(w, h) {
				const orig = this._origPath
				const info = this._origInfo
				if (!orig || !info || !this.mode) {
					return
				}
				const token = ++this._applyToken
				this.busy = true
				this.outW = w
				this.outH = h
				try {
					await this.$nextTick()
					const r = await encodeOnCanvas(this, 'work', orig, this.mode, w, h, info)
					if (token !== this._applyToken) {
						return
					}
					this._jpegU8 = r.u8
					this.hasJpeg = true
					this.dstPath = r.preview
					this.sentSeq = 0
					this.chunkCnt = 0
					this.log(this.rightTag + ' → ' + r.u8.length + ' B  q' + r.quality)
				} catch (e) {
					if (token !== this._applyToken) {
						return
					}
					this.log('处理失败: ' + this.errText(e))
				} finally {
					if (token === this._applyToken) {
						this.busy = false
					}
				}
			},
			onStop() {
				if (!this.sending || this.stopping) {
					return
				}
				this.stopping = true
				this._abort.aborted = true
				ble.statusWaiter.reset()
				this.log('停止发送')
			},
			async onPush() {
				if (this.sending) {
					return
				}
				if (!this.hasJpeg || !this._jpegU8) {
					uni.showToast({
						title: '请先选择图片并完成裁剪或缩放',
						icon: 'none'
					})
					return
				}
				if (!this.ready) {
					uni.showToast({
						title: '当前设备无 FF00 推图服务，无法发送',
						icon: 'none'
					})
					this.log('无法发送：当前 GATT 未发现 FF00/FF01/FF02/FF03')
					return
				}
				this._abort = { aborted: false }
				this.busy = true
				this.sending = true
				this.stopping = false
				this.sentSeq = 0
				this.chunkCnt = 0
				this.log('开始推图')
				try {
					await pushJpeg(ble, this._jpegU8, this.outW, this.outH, (m) => this.log(m), (p) => {
						this.sentSeq = p.seq
						this.chunkCnt = p.chunkCnt
					}, this._abort)
					this.log('DONE 推图成功')
					uni.showToast({
						title: '推图成功',
						icon: 'success'
					})
				} catch (e) {
					if (isAbortErr(e)) {
						this.log('已停止发送')
						uni.showToast({
							title: '已停止发送',
							icon: 'none'
						})
					} else {
						this.log('推图失败: ' + this.errText(e))
						uni.showToast({
							title: '推图失败',
							icon: 'none'
						})
					}
				} finally {
					this.busy = false
					this.sending = false
					this.stopping = false
				}
			}
		}
	}
</script>

<style>
	.page {
		min-height: 100vh;
		background: #f2f3f5;
		padding: 24rpx 24rpx 48rpx;
		box-sizing: border-box;
	}

	.pick {
		margin-bottom: 16rpx;
	}

	.modes,
	.sizes {
		display: flex;
		flex-wrap: wrap;
		margin-bottom: 12rpx;
	}

	.chip {
		padding: 12rpx 24rpx;
		border-radius: 999rpx;
		background: #fff;
		margin-right: 12rpx;
		margin-bottom: 12rpx;
		font-size: 26rpx;
		color: #333;
	}

	.chip.on {
		background: #007aff;
		color: #fff;
	}

	.custom {
		display: flex;
		align-items: center;
		margin-bottom: 16rpx;
	}

	.inp {
		flex: 1;
		height: 64rpx;
		background: #fff;
		border-radius: 10rpx;
		padding: 0 16rpx;
	}

	.x {
		margin: 0 12rpx;
		color: #888;
	}

	.ok {
		margin-left: 12rpx;
	}

	.block {
		margin-bottom: 24rpx;
	}

	.tag {
		display: block;
		font-size: 24rpx;
		color: #888;
		margin-bottom: 8rpx;
	}

	.tag-r {
		color: #007aff;
		font-weight: 600;
	}

	.pic {
		width: 100%;
		border-radius: 16rpx;
		display: block;
		background: transparent;
	}

	.ph {
		display: block;
		font-size: 24rpx;
		color: #888;
		padding: 48rpx 32rpx;
		text-align: center;
		background: #fff;
		border-radius: 16rpx;
	}

	.warn {
		display: block;
		font-size: 22rpx;
		color: #e64545;
		margin-bottom: 8rpx;
	}

	.prog {
		margin-bottom: 16rpx;
	}

	.prog-txt {
		display: block;
		text-align: center;
		font-size: 22rpx;
		color: #888;
		margin-top: 6rpx;
	}

	.log-card {
		background: #fff;
		border-radius: 16rpx;
		padding: 20rpx;
		height: 220rpx;
		margin-bottom: 20rpx;
	}

	.sec {
		display: block;
		font-size: 24rpx;
		color: #888;
		margin-bottom: 8rpx;
	}

	.log {
		height: 160rpx;
	}

	.log-line {
		display: block;
		font-size: 22rpx;
		color: #444;
		line-height: 1.5;
	}

	.send {
		margin-top: 8rpx;
	}

	.work {
		position: fixed;
		left: 0;
		top: 0;
		opacity: 0;
		pointer-events: none;
		z-index: -1;
	}

	.mask {
		position: fixed;
		left: 0;
		top: 0;
		right: 0;
		bottom: 0;
		background: rgba(0, 0, 0, 0.45);
		z-index: 999;
		display: flex;
		align-items: center;
		justify-content: center;
		padding: 48rpx;
		box-sizing: border-box;
	}

	.dlg {
		width: 100%;
		background: #fff;
		border-radius: 24rpx;
		padding: 40rpx 36rpx 32rpx;
		box-sizing: border-box;
	}

	.dlg-t {
		display: block;
		text-align: center;
		font-size: 32rpx;
		font-weight: 600;
		color: #111;
		margin-bottom: 32rpx;
	}

	.dlg-bar {
		display: flex;
		align-items: center;
	}

	.dlg-p {
		flex: 1;
	}

	.dlg-pct {
		width: 80rpx;
		text-align: right;
		font-size: 24rpx;
		color: #666;
		margin-left: 16rpx;
	}

	.dlg-sub {
		display: block;
		text-align: center;
		font-size: 22rpx;
		color: #999;
		margin-top: 16rpx;
	}

	.dlg-stop {
		margin-top: 32rpx;
		background: #f2f3f5;
		color: #333;
	}
</style>
