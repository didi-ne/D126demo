<template>
	<view class="page">
		<view class="head">
			<view class="head-l">
				<text class="dev">{{ deviceName }}</text>
				<text class="sub">{{ subLine }}</text>
			</view>
			<button class="disc" size="mini" @click="onDisconnect">断开连接</button>
		</view>

		<view class="tabs">
			<view class="tab" :class="{ on: tab === 'gatt' }" @click="tab = 'gatt'">蓝牙服务</view>
			<view class="tab" :class="{ on: tab === 'log' }" @click="openLiveLog">实时日志</view>
		</view>

		<view v-if="tab === 'gatt'" class="gatt">
			<view v-if="!gattTree.length" class="empty-g">未发现 GATT 服务</view>
			<view v-for="s in gattTree" :key="s.uuid" class="svc" :class="{ push: s.isPush, open: isOpen(s) }">
				<view class="svc-hd" @click="toggleSvc(s)">
					<view class="svc-hd-l">
						<text class="svc-t">{{ gattName(s.uuid) }}  ·  {{ uuidShort(s.uuid) }}</text>
						<text class="svc-u">{{ s.uuid }}</text>
					</view>
					<text class="exp">{{ isOpen(s) ? '收起' : '展开' }}</text>
				</view>
				<view v-if="isOpen(s)" class="svc-body">
					<view v-if="!s.chars.length" class="empty-g">该服务下没有特征</view>
					<view
						class="ch"
						v-for="c in s.chars"
						:key="c.uuid"
						:class="{ on: isView(s, c), img: isImgChar(c.uuid) }"
						@click="onPickChar(s, c)"
					>
						<view class="ch-l">
							<text class="ch-n">{{ gattName(c.uuid) }}</text>
							<text class="ch-u">{{ c.uuid }}</text>
						</view>
						<text class="ch-p">{{ charPropsText(c.properties) }}</text>
					</view>
					<view class="rx-box">
						<view class="rx-bar">
							<picker :range="rxModes" :value="rxModeIndex" @change="onRxMode">
								<view class="mode">RX:{{ rxModes[rxModeIndex] }}</view>
							</picker>
							<view class="tog" :class="{ on: notifyOn && isViewSvc(s) }" @click="onToggleNotify(s)">
								<text>通知:{{ notifyOn && isViewSvc(s) ? '开' : '关' }}</text>
							</view>
							<button class="op" size="mini" :disabled="busy" @click="onRead(s)">读取</button>
							<button class="op img-btn" size="mini" type="primary" :disabled="busy" @click="goSend(s)">发送图片</button>
						</view>
						<scroll-view scroll-y class="rx-val">
							<text class="rx-txt">{{ isViewSvc(s) ? (rxText || '—') : '—' }}</text>
						</scroll-view>
					</view>
					<view class="tx">
						<view class="tx-row">
							<picker :range="txModes" :value="txModeIndex" @change="onTxMode">
								<view class="mode">TX:{{ txModes[txModeIndex] }}</view>
							</picker>
							<input class="tx-inp" v-model="txText" placeholder="发送内容（支持换行与 \\r\\n\\t，HEX 支持空格/0x前缀）" />
							<button class="op" size="mini" type="primary" :disabled="busy || !sendOn" @click="onTxOnce(s)">发送</button>
						</view>
						<view class="tx-row tx-tools">
							<view class="tog" :class="{ on: sendOn }" @click="sendOn = !sendOn">
								<text>发送:{{ sendOn ? '开' : '关' }}</text>
							</view>
							<view class="tog" :class="{ on: loopOn }" @click="onToggleLoop(s)">
								<text>循环发送</text>
							</view>
							<input class="ms" type="number" v-model="loopMs" />
							<button class="op" size="mini" :disabled="busy" @click="onQuick">快捷</button>
							<button class="op" size="mini" :disabled="busy" @click="onHist">历史</button>
						</view>
					</view>
				</view>
			</view>
		</view>

		<view v-else class="log-pane">
			<view class="log-ops">
				<button class="op" size="mini" @click="onClearLogs">清空</button>
				<button class="op" size="mini" :class="{ onbtn: logMode === 'hist' }" @click="onShowHistLogs">历史</button>
			</view>
			<scroll-view scroll-y class="log">
				<text v-if="!displayLogs.length" class="log-empty">{{ logMode === 'hist' ? '暂无历史日志' : '暂无实时日志' }}</text>
				<view class="log-row" :class="'k-' + item.kind" v-for="(item, i) in displayLogs" :key="i">
					<text class="log-tag">{{ item.tag }}</text>
					<view class="log-body">
						<text class="log-t">{{ item.t }}</text>
						<text class="log-msg">{{ item.text }}</text>
					</view>
				</view>
			</scroll-view>
		</view>
	</view>
</template>

<script>
	import { ble } from '../../common/session.js'
	import { gattName, uuidShort, charPropsText, sameUuid, UUID_CTRL, UUID_DATA, UUID_STATUS } from '../../common/proto.js'
	import { formatRx, encodeTx, bytesToHex } from '../../common/packet.js'

	const TX_HIST_KEY = 'ble_img_tx_hist'
	const TX_QUICK_KEY = 'ble_img_tx_quick'
	const LOG_HIST_KEY = 'ble_img_log_hist'

	export default {
		data() {
			return {
				busy: false,
				ready: false,
				deviceName: '',
				mtu: 0,
				protoReady: false,
				tab: 'gatt',
				logMode: 'live',
				logs: [],
				histLogs: [],
				txModes: ['HEX', 'UTF-8', 'ASCII', 'Base64'],
				txModeIndex: 0,
				txText: '',
				sendOn: true,
				loopOn: false,
				loopMs: '200',
				gattTree: [],
				txServiceId: '',
				txCharId: '',
				txWriteType: 'writeNoResponse',
				viewServiceId: '',
				viewCharId: '',
				rxModes: ['HEX', 'UTF-8', 'ASCII', 'Base64'],
				rxModeIndex: 0,
				rxBytes: null,
				rxText: '',
				notifyOn: false,
				openSvcId: '',
				selCharId: {}
			}
		},
		computed: {
			subLine() {
				const mtu = this.mtu ? 'MTU ' + this.mtu : 'MTU —'
				const svc = this.protoReady ? '推图服务已就绪' : '未发现 FF00 推图服务'
				return mtu + '  ·  ' + svc
			},
			displayLogs() {
				const src = this.logMode === 'hist' ? this.histLogs : this.logs
				return src.map((x) => this.parseLogLine(x))
			}
		},
		onLoad() {
			this._loopTimer = null
			this._loopSvc = null
			this._leaving = false
			this.deviceName = ble.deviceName
			this.mtu = ble.mtu
			this.ready = ble.ready
			this.protoReady = ble.protoReady
			this.gattTree = ble.gattTree || []
			this.openSvcId = ''
			this.selCharId = {}
			if (!ble.connected) {
				uni.showToast({
					title: '设备未连接',
					icon: 'none'
				})
				uni.navigateBack()
				return
			}
			this.log(this.protoReady ? '推图服务已就绪' : '已连接，未发现 FF00 推图服务')
			const buffered = ble.takeLogs()
			for (let i = 0; i < buffered.length; i++) {
				this.log(buffered[i])
			}
			ble.onLog((msg) => this.log(msg))
			ble.onConnectionChange((ok) => {
				this.ready = ok && ble.protoReady
				this.protoReady = ble.protoReady
				if (!ok && !this._leaving) {
					this.log('链路断开')
					this._leaving = true
					uni.reLaunch({
						url: '/pages/index/index'
					})
				}
			})
			ble.onRx((uuid, u8) => {
				const hex = bytesToHex(u8)
				this.log('RX[' + uuid + '] hex=' + hex)
				if (sameUuid(uuid, this.viewCharId)) {
					this.rxBytes = u8
					this.refreshRx()
				}
			})
		},
		onUnload() {
			this._leaving = true
			this.stopLoop()
			ble.onRx(null)
			ble.onLog(null)
			ble.onConnectionChange(null)
			if (ble.connected) {
				ble.disconnect()
			}
		},
		methods: {
			gattName,
			uuidShort,
			charPropsText,
			classifyLog(msg) {
				const s = String(msg || '')
				if (/失败|无法|断开|超时|error|Error/i.test(s)) {
					return { kind: 'err', tag: 'ERROR' }
				}
				if (/未发现 FF00|发送功能已关闭/.test(s)) {
					return { kind: 'warn', tag: 'WARNING' }
				}
				if (/^TX\b/.test(s)) {
					return { kind: 'tx', tag: 'TX' }
				}
				if (/^RX\[/.test(s)) {
					return { kind: 'rx', tag: 'RX' }
				}
				if (/通知/.test(s)) {
					return { kind: 'ntf', tag: 'NOTIFY' }
				}
				if (/读取/.test(s)) {
					return { kind: 'rd', tag: 'READ' }
				}
				if (/循环发送/.test(s)) {
					return { kind: 'loop', tag: 'LOOPING' }
				}
				if (/已选择特征/.test(s)) {
					return { kind: 'sel', tag: 'SELECT' }
				}
				if (/推图.*就绪|已就绪/.test(s)) {
					return { kind: 'ok', tag: 'PUSH READY' }
				}
				return { kind: 'info', tag: 'INFO' }
			},
			parseLogLine(line) {
				if (line && typeof line === 'object' && line.text != null) {
					return line
				}
				const raw = String(line || '')
				const m = raw.match(/^(\[[^\]]+\])\s*(.*)$/)
				const t = m ? m[1] : ''
				const text = m ? m[2] : raw
				const meta = this.classifyLog(text)
				return { t, kind: meta.kind, tag: meta.tag, text }
			},
			log(msg) {
				const d = new Date()
				const pad = (n) => (n < 10 ? '0' + n : '' + n)
				const ms = ('00' + d.getMilliseconds()).slice(-3)
				const t = '[' + pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds()) + '.' + ms + ']'
				const meta = this.classifyLog(msg)
				const item = { t, kind: meta.kind, tag: meta.tag, text: String(msg) }
				this.logs = [item].concat(this.logs).slice(0, 200)
				let hist = []
				try {
					hist = uni.getStorageSync(LOG_HIST_KEY) || []
				} catch (e) {
					hist = []
				}
				if (!Array.isArray(hist)) {
					hist = []
				}
				hist.unshift(t + ' ' + msg)
				uni.setStorageSync(LOG_HIST_KEY, hist.slice(0, 500))
			},
			onClearLogs() {
				if (this.logMode === 'hist') {
					uni.showModal({
						title: '清空历史日志',
						content: '将删除本地保存的历史日志',
						success: (res) => {
							if (!res.confirm) {
								return
							}
							uni.removeStorageSync(LOG_HIST_KEY)
							this.histLogs = []
						}
					})
					return
				}
				this.logs = []
			},
			onShowHistLogs() {
				if (this.logMode === 'hist') {
					this.logMode = 'live'
					return
				}
				let hist = []
				try {
					hist = uni.getStorageSync(LOG_HIST_KEY) || []
				} catch (e) {
					hist = []
				}
				this.histLogs = Array.isArray(hist) ? hist : []
				this.logMode = 'hist'
			},
			openLiveLog() {
				this.tab = 'log'
				this.logMode = 'live'
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
			refreshRx() {
				this.rxText = this.rxBytes ? formatRx(this.rxBytes, this.rxModes[this.rxModeIndex]) : ''
			},
			isOpen(s) {
				return this.openSvcId === s.uuid
			},
			isViewSvc(s) {
				return s.uuid === this.viewServiceId
			},
			isView(s, c) {
				return s.uuid === this.viewServiceId && c.uuid === this.viewCharId
			},
			isImgChar(uuid) {
				return sameUuid(uuid, UUID_CTRL) || sameUuid(uuid, UUID_DATA) || sameUuid(uuid, UUID_STATUS)
			},
			charIn(s) {
				const id = this.selCharId[s.uuid]
				if (!id) {
					return null
				}
				return (s.chars || []).find((c) => c.uuid === id) || null
			},
			propsOf(s) {
				const c = this.charIn(s)
				return (c && c.properties) || {}
			},
			denyPerm(title, content) {
				uni.showModal({
					title,
					content,
					showCancel: false,
					confirmText: '确定'
				})
				this.log(title + '：' + content)
			},
			needChar(s) {
				const c = this.charIn(s)
				if (c) {
					return c
				}
				this.denyPerm('未选择特征', '请先在该服务下选择一个特征')
				return null
			},
			isNoPerm(e) {
				const t = this.errText(e)
				return (e && e.errCode === 10007) || /property not support|10007/i.test(t)
			},
			toggleSvc(s) {
				this.openSvcId = this.openSvcId === s.uuid ? '' : s.uuid
			},
			onPickChar(s, c) {
				this.selCharId = Object.assign({}, this.selCharId, { [s.uuid]: c.uuid })
				this.viewServiceId = s.uuid
				this.viewCharId = c.uuid
				this.notifyOn = false
				const p = c.properties || {}
				this.log('已选择特征 ' + s.uuid + ' | ' + c.uuid + ' 权限 ' + charPropsText(p))
				if (p.write || p.writeNoResponse) {
					this.txServiceId = s.uuid
					this.txCharId = c.uuid
					this.txWriteType = p.writeNoResponse ? 'writeNoResponse' : 'write'
				}
			},
			onRxMode(e) {
				this.rxModeIndex = Number(e.detail.value)
				this.refreshRx()
			},
			async onRead(s) {
				const c = this.needChar(s)
				if (!c) {
					return
				}
				if (!(c.properties || {}).read) {
					this.denyPerm('无法读取', '该特征不具备 Read 权限')
					return
				}
				this.busy = true
				this.log('读取请求已发送: ' + s.uuid + ' | ' + c.uuid)
				this.openLiveLog()
				try {
					await ble.readChar(s.uuid, c.uuid)
					this.log('读取请求已完成，等待特征值回报')
				} catch (e) {
					this.log('读取失败: ' + this.errText(e))
					if (this.isNoPerm(e)) {
						this.denyPerm('无法读取', '该特征不具备 Read 权限')
					} else {
						uni.showToast({
							title: this.errText(e),
							icon: 'none'
						})
					}
				} finally {
					this.busy = false
				}
			},
			async onToggleNotify(s) {
				const c = this.needChar(s)
				if (!c) {
					return
				}
				const p = c.properties || {}
				if (!p.notify && !p.indicate) {
					this.denyPerm('无法开启通知', '该特征不具备 Notify / Indicate 权限')
					return
				}
				const next = !(this.notifyOn && this.isViewSvc(s))
				this.busy = true
				try {
					await ble.notifyChar(s.uuid, c.uuid, next)
					this.viewServiceId = s.uuid
					this.viewCharId = c.uuid
					this.notifyOn = next
					this.log('通知已' + (next ? '开启' : '关闭') + ': ' + c.uuid)
					this.openLiveLog()
				} catch (e) {
					this.log('通知失败: ' + this.errText(e))
					this.openLiveLog()
					if (this.isNoPerm(e)) {
						this.denyPerm('无法开启通知', '该特征不具备 Notify / Indicate 权限')
					} else {
						uni.showToast({
							title: this.errText(e),
							icon: 'none'
						})
					}
				} finally {
					this.busy = false
				}
			},
			goSend() {
				if (!this.protoReady) {
					this.denyPerm('无法发送图片', '当前设备未提供 FF00 推图服务（CTRL/DATA/STATUS）')
					return
				}
				uni.navigateTo({
					url: '/pages/send/send'
				})
			},
			async onDisconnect() {
				this._leaving = true
				this.stopLoop()
				await ble.disconnect()
				uni.navigateBack()
			},
			onTxMode(e) {
				this.txModeIndex = Number(e.detail.value)
			},
			buildPayload() {
				return encodeTx(this.txText, this.txModes[this.txModeIndex])
			},
			rememberTx() {
				const item = {
					mode: this.txModes[this.txModeIndex],
					text: this.txText,
					ts: Date.now()
				}
				let hist = []
				try {
					hist = uni.getStorageSync(TX_HIST_KEY) || []
				} catch (e) {
					hist = []
				}
				hist.unshift(item)
				uni.setStorageSync(TX_HIST_KEY, hist.slice(0, 30))
			},
			async onTxOnce(s) {
				if (!this.sendOn) {
					return
				}
				const svc = s || this.gattTree.find((x) => x.uuid === this.viewServiceId)
				if (!svc) {
					this.denyPerm('无法发送', '请先展开服务并选择特征')
					return
				}
				const c = this.needChar(svc)
				if (!c) {
					return
				}
				const p = c.properties || {}
				if (!p.write && !p.writeNoResponse) {
					this.denyPerm('无法发送', '该特征不具备 Write / Write Without Response 权限')
					return
				}
				const writeType = p.writeNoResponse ? 'writeNoResponse' : 'write'
				this.busy = !this.loopOn
				try {
					const u8 = this.buildPayload()
					await ble.writeRaw(u8, svc.uuid, c.uuid, writeType)
					this.rememberTx()
					this.log('TX ' + this.txModes[this.txModeIndex] + ' ' + u8.length + 'B hex=' + bytesToHex(u8))
					if (!this.loopOn) {
						this.openLiveLog()
					}
				} catch (e) {
					this.log('TX 失败: ' + this.errText(e))
					if (!this.loopOn) {
						this.openLiveLog()
					}
					if (this.isNoPerm(e)) {
						this.denyPerm('无法发送', '该特征不具备写入权限')
					} else {
						uni.showToast({
							title: this.errText(e),
							icon: 'none'
						})
					}
				} finally {
					if (!this.loopOn) {
						this.busy = false
					}
				}
			},
			stopLoop() {
				this.loopOn = false
				this._loopSvc = null
				if (this._loopTimer) {
					clearInterval(this._loopTimer)
					this._loopTimer = null
				}
			},
			onToggleLoop(s) {
				if (this.loopOn) {
					this.stopLoop()
					this.log('已关闭循环发送')
					this.openLiveLog()
					return
				}
				if (!this.sendOn) {
					uni.showToast({
						title: '发送功能已关闭',
						icon: 'none'
					})
					return
				}
				const svc = s || this.gattTree.find((x) => x.uuid === this.viewServiceId)
				if (!svc || !this.charIn(svc)) {
					this.denyPerm('无法发送', '请先在该服务下选择一个可写特征')
					return
				}
				const p = this.propsOf(svc)
				if (!p.write && !p.writeNoResponse) {
					this.denyPerm('无法发送', '该特征不具备 Write / Write Without Response 权限')
					return
				}
				const ms = parseInt(this.loopMs, 10) || 200
				this.loopOn = true
				this._loopSvc = svc
				this._loopTimer = setInterval(() => {
					if (!this.sendOn) {
						return
					}
					this.onTxOnce(this._loopSvc)
				}, ms)
				this.log('循环发送 ' + ms + 'ms')
				this.openLiveLog()
			},
			onQuick() {
				let list = []
				try {
					list = uni.getStorageSync(TX_QUICK_KEY) || []
				} catch (e) {
					list = []
				}
				if (!list.length) {
					const hist = uni.getStorageSync(TX_HIST_KEY) || []
					list = hist.slice(0, 5).map((x) => x.text).filter(Boolean)
				}
				if (!list.length) {
					uni.showToast({
						title: '暂无快捷内容，请先成功发送一次',
						icon: 'none'
					})
					return
				}
				uni.showActionSheet({
					itemList: list.map((t) => t.length > 24 ? t.slice(0, 24) + '…' : t),
					success: (res) => {
						this.txText = list[res.tapIndex]
					}
				})
			},
			onHist() {
				let hist = []
				try {
					hist = uni.getStorageSync(TX_HIST_KEY) || []
				} catch (e) {
					hist = []
				}
				if (!hist.length) {
					uni.showToast({
						title: '暂无发送历史',
						icon: 'none'
					})
					return
				}
				uni.showActionSheet({
					itemList: hist.slice(0, 8).map((x) => (x.mode + ' ' + x.text).slice(0, 28)),
					success: (res) => {
						const it = hist[res.tapIndex]
						this.txText = it.text
						const name = it.mode === 'TEXT' ? 'UTF-8' : it.mode
						const idx = this.txModes.indexOf(name)
						this.txModeIndex = idx >= 0 ? idx : 0
					}
				})
			}
		}
	}
</script>

<style>
	.page {
		min-height: 100vh;
		background: #f2f3f5;
		padding: 24rpx 24rpx 40rpx;
		box-sizing: border-box;
		display: flex;
		flex-direction: column;
	}

	.head {
		display: flex;
		align-items: center;
		background: #fff;
		border-radius: 16rpx;
		padding: 24rpx;
	}

	.head-l {
		flex: 1;
		min-width: 0;
		padding-right: 16rpx;
	}

	.dev {
		display: block;
		font-size: 32rpx;
		font-weight: 600;
		color: #111;
	}

	.sub {
		display: block;
		margin-top: 8rpx;
		font-size: 22rpx;
		color: #888;
	}

	.disc {
		margin: 0;
		flex-shrink: 0;
		margin-left: 12rpx;
	}

	.tabs {
		display: flex;
		margin-top: 20rpx;
		padding: 0 4rpx;
	}

	.tab {
		padding: 12rpx 28rpx;
		border-radius: 999rpx;
		background: #fff;
		font-size: 26rpx;
		color: #333;
		margin-right: 16rpx;
		border: 2rpx solid #e8e8e8;
	}

	.tab.on {
		background: #e8f1ff;
		color: #007aff;
		border-color: #007aff;
	}

	.gatt {
		margin-top: 20rpx;
		background: transparent;
		padding: 0;
	}

	.empty-g {
		font-size: 24rpx;
		color: #999;
		padding: 24rpx 8rpx;
		background: #fff;
		border-radius: 16rpx;
	}

	.svc {
		background: #fff;
		border: 1rpx solid #f0f0f0;
		border-radius: 16rpx;
		padding: 24rpx;
		margin-bottom: 28rpx;
	}

	.svc.push {
		border-color: #007aff;
		background: #f7fbff;
	}

	.svc-hd {
		display: flex;
		align-items: center;
	}

	.svc-hd-l {
		flex: 1;
		min-width: 0;
		padding-right: 16rpx;
	}

	.exp {
		flex-shrink: 0;
		font-size: 24rpx;
		color: #007aff;
		padding: 8rpx 4rpx;
	}

	.svc-t {
		display: block;
		font-size: 26rpx;
		font-weight: 600;
		color: #111;
	}

	.svc-u {
		display: block;
		font-size: 20rpx;
		color: #999;
		margin-top: 8rpx;
		word-break: break-all;
	}

	.svc-body {
		margin-top: 16rpx;
		padding-top: 8rpx;
		border-top: 1rpx solid #f0f0f0;
	}

	.ch {
		display: flex;
		align-items: center;
		padding: 14rpx 12rpx;
		border-radius: 10rpx;
		background: #f7f7f8;
		margin-top: 8rpx;
	}

	.ch.img {
		background: #eef6ff;
	}

	.ch.on {
		background: #007aff;
	}

	.ch-l {
		flex: 1;
		min-width: 0;
		padding-right: 12rpx;
	}

	.ch-n {
		display: block;
		font-size: 24rpx;
		color: #222;
	}

	.ch.on .ch-n,
	.ch.on .ch-u,
	.ch.on .ch-p {
		color: #fff;
	}

	.ch-u {
		display: block;
		font-size: 18rpx;
		color: #999;
		word-break: break-all;
	}

	.ch-p {
		font-size: 20rpx;
		color: #007aff;
		flex-shrink: 0;
	}

	.rx-box {
		margin-top: 12rpx;
	}

	.rx-bar {
		display: flex;
		align-items: center;
		flex-wrap: wrap;
	}

	.img-btn {
		margin-left: 8rpx;
		margin-top: 8rpx;
	}

	.rx-val {
		margin-top: 12rpx;
		height: 240rpx;
		background: #111;
		border-radius: 10rpx;
		padding: 12rpx;
	}

	.rx-txt {
		font-size: 22rpx;
		color: #9f9;
		line-height: 1.4;
		word-break: break-all;
	}

	.log-pane {
		margin-top: 20rpx;
		background: #fff;
		border-radius: 16rpx;
		padding: 20rpx 16rpx 16rpx;
		flex: 1;
		display: flex;
		flex-direction: column;
		min-height: 0;
	}

	.log-ops {
		display: flex;
		align-items: center;
		margin-bottom: 16rpx;
		flex-shrink: 0;
	}

	.log-ops .op {
		margin-right: 12rpx;
	}

	.onbtn {
		background: #e8f1ff;
		color: #007aff;
	}

	.log {
		flex: 1;
		height: 72vh;
	}

	.log-empty {
		display: block;
		font-size: 26rpx;
		color: #999;
		padding: 32rpx 12rpx;
	}

	.log-row {
		display: flex;
		align-items: flex-start;
		padding: 18rpx 16rpx;
		border-radius: 12rpx;
		margin-bottom: 14rpx;
		background: #f7f8fa;
	}

	.log-tag {
		flex-shrink: 0;
		min-width: 120rpx;
		padding: 0 12rpx;
		height: 40rpx;
		line-height: 40rpx;
		text-align: center;
		border-radius: 8rpx;
		font-size: 18rpx;
		font-weight: 700;
		letter-spacing: 0.5rpx;
		margin-right: 16rpx;
		margin-top: 4rpx;
		color: #fff;
		background: #667085;
	}

	.log-body {
		flex: 1;
		min-width: 0;
	}

	.log-t {
		display: block;
		font-size: 20rpx;
		color: #98a2b3;
		margin-bottom: 6rpx;
	}

	.log-msg {
		display: block;
		font-size: 26rpx;
		line-height: 1.55;
		color: #344054;
		word-break: break-all;
	}

	.k-tx {
		background: #eef6ff;
	}
	.k-tx .log-tag {
		background: #1570ef;
	}
	.k-tx .log-msg {
		color: #175cd3;
	}

	.k-rx {
		background: #ecfdf3;
	}
	.k-rx .log-tag {
		background: #12b76a;
	}
	.k-rx .log-msg {
		color: #027a48;
	}

	.k-err {
		background: #fef3f2;
	}
	.k-err .log-tag {
		background: #d92d20;
	}
	.k-err .log-msg {
		color: #b42318;
		font-weight: 600;
	}

	.k-warn {
		background: #fffaeb;
	}
	.k-warn .log-tag {
		background: #dc6803;
	}
	.k-warn .log-msg {
		color: #b54708;
	}

	.k-ntf {
		background: #f4f3ff;
	}
	.k-ntf .log-tag {
		background: #7a5af8;
	}
	.k-ntf .log-msg {
		color: #5925dc;
	}

	.k-rd {
		background: #eefcff;
	}
	.k-rd .log-tag {
		background: #088ab2;
	}
	.k-rd .log-msg {
		color: #026aa2;
	}

	.k-sel {
		background: #f8f9fc;
	}
	.k-sel .log-tag {
		background: #667085;
	}

	.k-loop {
		background: #fdf2fa;
	}
	.k-loop .log-tag {
		background: #c11574;
	}
	.k-loop .log-msg {
		color: #9e165f;
	}

	.k-ok {
		background: #ecfdf3;
	}
	.k-ok .log-tag {
		background: #039855;
	}
	.k-ok .log-msg {
		color: #027a48;
	}

	.k-info .log-tag {
		background: #98a2b3;
	}

	.tx {
		margin-top: 16rpx;
		background: #f7f7f8;
		border-radius: 12rpx;
		padding: 16rpx;
	}

	.tx-row {
		display: flex;
		align-items: center;
	}

	.tx-tools {
		margin-top: 12rpx;
	}

	.mode {
		width: 220rpx;
		height: 64rpx;
		line-height: 64rpx;
		text-align: center;
		background: #f2f3f5;
		border-radius: 10rpx;
		font-size: 24rpx;
		color: #333;
		margin-right: 12rpx;
	}

	.tx-inp {
		flex: 1;
		height: 64rpx;
		background: #f2f3f5;
		border-radius: 10rpx;
		padding: 0 16rpx;
		font-size: 26rpx;
		margin-right: 12rpx;
	}

	.op {
		margin: 0;
		flex-shrink: 0;
	}

	.tog {
		padding: 10rpx 16rpx;
		border-radius: 10rpx;
		background: #f2f3f5;
		margin-right: 12rpx;
	}

	.tog.on {
		background: #e8f1ff;
	}

	.tog text {
		font-size: 22rpx;
		color: #333;
	}

	.ms {
		width: 100rpx;
		height: 56rpx;
		background: #f2f3f5;
		border-radius: 10rpx;
		text-align: center;
		font-size: 24rpx;
		margin-right: 12rpx;
	}
</style>
