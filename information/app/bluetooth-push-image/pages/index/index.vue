<template>
	<view class="page">
		<view class="toolbar">
			<button class="tb-btn" size="mini" :disabled="busy" @click="onRefresh">刷新</button>
			<button class="tb-btn" size="mini" type="primary" :disabled="busy || scanning" @click="onStart">开始扫描</button>
			<button class="tb-btn" size="mini" :disabled="busy || !scanning" @click="onStop">停止扫描</button>
		</view>
		<text class="hint">{{ statusText }}</text>

		<view v-if="history.length" class="section">
			<text class="sec-title">历史连接</text>
			<view class="card">
				<view
					class="row"
					v-for="(d, i) in history"
					:key="'h-' + d.deviceId"
					:class="{ last: i === history.length - 1 }"
				>
					<device-icon :kind="kindOf(d)"></device-icon>
					<view class="meta">
						<text class="name">{{ d.name }}</text>
						<text class="id">{{ d.deviceId }}</text>
					</view>
					<button class="link ghost" size="mini" :disabled="busy" @click="onForget(d)">忘记</button>
					<button class="link" size="mini" type="primary" :disabled="busy" @click="onConnect(d)">连接</button>
				</view>
			</view>
		</view>

		<view class="section">
			<text class="sec-title">可用设备 {{ devices.length ? '(' + devices.length + ')' : '' }}</text>
			<view v-if="!devices.length" class="empty">{{ scanning ? '正在扫描…' : '请点击开始扫描，从列表中选择要连接的设备' }}</view>
			<view v-else class="card">
				<view
					class="row"
					v-for="(d, i) in devices"
					:key="d.deviceId"
					:class="{ last: i === devices.length - 1 }"
				>
					<device-icon :kind="kindOf(d)"></device-icon>
					<view class="meta">
						<text class="name">{{ d.name }}</text>
						<text class="id">{{ d.deviceId }}{{ d.RSSI != null ? '  |  RSSI ' + d.RSSI : '' }}</text>
					</view>
					<button class="link" size="mini" type="primary" :disabled="busy" @click="onConnect(d)">连接</button>
				</view>
			</view>
		</view>
	</view>
</template>

<script>
	import { ble } from '../../common/session.js'
	import { loadHist, saveHist, forgetHist, isConnectAbort } from '../../common/ble.js'
	import { deviceKind, bleErrText, isBtOffErr, isSysOccupiedErr } from '../../common/proto.js'
	import DeviceIcon from '../../components/device-icon.vue'

	export default {
		components: {
			DeviceIcon
		},
		data() {
			return {
				busy: false,
				scanning: false,
				btOn: false,
				history: [],
				devices: []
			}
		},
		computed: {
			statusText() {
				if (!this.btOn) {
					return '蓝牙未开启'
				}
				if (this.busy) {
					return '连接中…'
				}
				if (this.scanning) {
					return '正在扫描，请点击设备右侧的连接'
				}
				return '蓝牙已打开'
			}
		},
		onLoad() {
			this._map = {}
			this.history = this.namedOnly(loadHist())
			ble.onAdapterChange((on) => {
				this.btOn = on
				if (!on) {
					this.scanning = false
				}
			})
			this.syncAdapter(false)
		},
		onShow() {
			this.syncAdapter(false)
		},
		onBackPress() {
			if (!this.busy) {
				return false
			}
			this._connectCancelled = true
			ble.abortConnect()
			this.busy = false
			uni.showToast({
				title: '已取消连接',
				icon: 'none'
			})
			return true
		},
		onUnload() {
			ble.onAdapterChange(null)
			ble.stopScan()
		},
		methods: {
			errText(e) {
				return bleErrText(e)
			},
			kindOf(d) {
				return deviceKind(d.name || d.localName)
			},
			namedOnly(list) {
				return (list || []).filter((x) => {
					const n = String(x.name || x.localName || '').trim()
					return n && n !== '未知设备'
				})
			},
			async syncAdapter(prompt) {
				try {
					await ble.ensureOpen()
					this.btOn = true
					return true
				} catch (e) {
					this.btOn = false
					this.scanning = false
					if (isBtOffErr(e) || /not init/i.test(this.errText(e))) {
						if (prompt) {
							await this.askEnableBt()
						}
						return false
					}
					if (prompt) {
						uni.showToast({
							title: this.errText(e),
							icon: 'none'
						})
					}
					return false
				}
			},
			askEnableBt() {
				return new Promise((resolve) => {
					uni.showModal({
						title: '蓝牙未开启',
						content: '刷新与扫描需要打开手机蓝牙',
						confirmText: '去开启',
						success: (res) => {
							if (res.confirm) {
								ble.openBtSettings()
							}
							resolve(!!res.confirm)
						},
						fail: () => resolve(false)
					})
				})
			},
			async openAdapter() {
				await this.syncAdapter(false)
			},
			upsert(d) {
				const incoming = String(d.localName || d.name || '').trim()
				const prev = this._map[d.deviceId]
				if (!incoming && (!prev || !prev.name || prev.name === '未知设备')) {
					return
				}
				const item = {
					deviceId: d.deviceId,
					name: incoming || prev.name,
					localName: d.localName || (prev && prev.localName),
					RSSI: d.RSSI != null ? d.RSSI : (prev && prev.RSSI)
				}
				this._map[d.deviceId] = item
				const list = Object.keys(this._map).map((k) => this._map[k])
					.filter((x) => x.name && x.name !== '未知设备')
				list.sort((a, b) => (b.RSSI || -999) - (a.RSSI || -999))
				this.devices = list
			},
			async onStart() {
				if (!(await this.syncAdapter(true))) {
					return
				}
				this.scanning = true
				try {
					await ble.startScan((d) => this.upsert(d))
					this.btOn = true
				} catch (e) {
					this.scanning = false
					if (isBtOffErr(e) || /not init/i.test(this.errText(e))) {
						this.btOn = false
						await this.askEnableBt()
						return
					}
					uni.showToast({
						title: this.errText(e),
						icon: 'none'
					})
				}
			},
			async onStop() {
				await ble.stopScan()
				this.scanning = false
			},
			async onRefresh() {
				this._map = {}
				this.devices = []
				await this.onStop()
				await this.onStart()
			},
			onForget(d) {
				uni.showModal({
					title: '忘记设备',
					content: '从历史连接中移除 ' + (d.name || d.deviceId) + '？',
					success: (res) => {
						if (!res.confirm) {
							return
						}
						this.history = this.namedOnly(forgetHist(d.deviceId))
					}
				})
			},
			showConnectFail(d, e) {
				const name = d.name || d.localName || d.deviceId
				if (isSysOccupiedErr(e)) {
					const prof = (e.profiles || []).join(' / ')
					uni.showModal({
						title: '无法连接',
						content: '「' + name + '」已由系统蓝牙连接' + (prof ? '（' + prof + '）' : '') + '。应用无法同时建立 GATT。请先在系统蓝牙设置中断开该设备，再返回本页点击开始扫描，出现在可用设备后再连接。',
						confirmText: '打开蓝牙设置',
						cancelText: '取消',
						success: (res) => {
							if (res.confirm) {
								ble.openBtPanel()
							}
						}
					})
					return
				}
				uni.showModal({
					title: '连接失败',
					content: this.errText(e),
					showCancel: false,
					confirmText: '确定'
				})
			},
			async onConnect(d) {
				if (this.busy) {
					return
				}
				if (!(await this.syncAdapter(true))) {
					return
				}
				this._connectCancelled = false
				this.busy = true
				try {
					await ble.connect(d)
					if (this._connectCancelled) {
						return
					}
					saveHist({
						deviceId: d.deviceId,
						localName: d.localName,
						name: d.name || d.localName
					})
					this.history = this.namedOnly(loadHist())
					this.scanning = false
					uni.navigateTo({
						url: '/pages/push/push'
					})
				} catch (e) {
					if (this._connectCancelled || isConnectAbort(e)) {
						if (!this._connectCancelled) {
							uni.showToast({
								title: '已取消连接',
								icon: 'none'
							})
						}
						return
					}
					this.showConnectFail(d, e)
				} finally {
					this.busy = false
					this.scanning = ble.scanning
				}
			}
		}
	}
</script>

<style>
	.page {
		min-height: 100vh;
		background: #f2f3f5;
		padding: 24rpx;
		box-sizing: border-box;
	}

	.toolbar {
		display: flex;
		background: #fff;
		border-radius: 16rpx;
		padding: 16rpx;
	}

	.tb-btn {
		flex: 1;
		margin: 0;
	}

	.tb-btn + .tb-btn {
		margin-left: 12rpx;
	}

	.hint {
		display: block;
		margin: 16rpx 8rpx 8rpx;
		font-size: 24rpx;
		color: #888;
	}

	.section {
		margin-top: 28rpx;
	}

	.sec-title {
		display: block;
		font-size: 24rpx;
		color: #888;
		margin: 0 8rpx 12rpx;
	}

	.card {
		background: #fff;
		border-radius: 16rpx;
		overflow: hidden;
	}

	.row {
		display: flex;
		align-items: center;
		padding: 24rpx;
		border-bottom: 1rpx solid #f0f0f0;
	}

	.row.last {
		border-bottom: none;
	}

	.meta {
		flex: 1;
		width: 0;
		min-width: 0;
		overflow: hidden;
		padding-right: 16rpx;
	}

	.name,
	.id {
		display: block;
		overflow: hidden;
		text-overflow: ellipsis;
		white-space: nowrap;
		width: 100%;
		lines: 1;
	}

	.name {
		font-size: 30rpx;
		color: #111;
		font-weight: 600;
	}

	.id {
		margin-top: 6rpx;
		font-size: 22rpx;
		color: #888;
	}

	.link {
		margin: 0;
		flex-shrink: 0;
	}

	.link.ghost {
		background: #f2f3f5;
		color: #666;
		margin-right: 12rpx;
	}

	.empty {
		background: #fff;
		border-radius: 16rpx;
		padding: 48rpx 24rpx;
		text-align: center;
		font-size: 26rpx;
		color: #999;
	}
</style>
