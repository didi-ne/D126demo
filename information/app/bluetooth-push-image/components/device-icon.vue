<template>
	<view class="box">
		<image class="img" :src="src" mode="aspectFit"></image>
	</view>
</template>

<script>
	const SVG = {
		ble: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#007AFF"><path d="M17.71 7.71L12 2h-1v7.59L6.41 5 5 6.41 10.59 12 5 17.59 6.41 19 11 14.41V22h1l5.71-5.71-4.3-4.29 4.3-4.29zM13 5.83l1.88 1.88L13 9.59V5.83zm1.88 10.46L13 18.17v-3.76l1.88 1.88z"/></svg>',
		earbuds: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#007AFF"><path d="M10 8v12c0 .55-.45 1-1 1H8c-.55 0-1-.45-1-1v-6.73C6.32 13.72 5.61 14 5 14c-2 0-3-2-3-3V6c0-1 1-3 3-3s5 3 5 5zm6.5-5C13.46 3 11 5.46 11 8.5S13.46 14 16.5 14 22 11.54 22 8.5 19.54 3 16.5 3zm-2 11.68V20c0 .55.45 1 1 1h2c.55 0 1-.45 1-1v-5.32c-.63.21-1.3.32-2 .32s-1.37-.11-2-.32z"/></svg>',
		overear: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#007AFF"><path d="M12 3a8 8 0 00-8 8v6a3 3 0 003 3h1v-8H6v-1a6 6 0 0112 0v1h-2v8h1a3 3 0 003-3v-6a8 8 0 00-8-8z"/></svg>',
		watch: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#007AFF"><path d="M8 2h8v2H8V2zm0 18h8v2H8v-2z"/><rect x="6" y="5" width="12" height="14" rx="3"/></svg>',
		speaker: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#007AFF"><rect x="7" y="2" width="10" height="20" rx="3"/><circle cx="12" cy="14" r="3.2" fill="#EAF3FF"/><circle cx="12" cy="6.5" r="1.3" fill="#EAF3FF"/></svg>'
	}

	export default {
		name: 'DeviceIcon',
		props: {
			kind: {
				type: String,
				default: 'ble'
			}
		},
		computed: {
			src() {
				const svg = SVG[this.kind] || SVG.ble
				return 'data:image/svg+xml;charset=utf-8,' + encodeURIComponent(svg)
			}
		}
	}
</script>

<style scoped>
	.box {
		width: 72rpx;
		height: 72rpx;
		flex-shrink: 0;
		margin-right: 20rpx;
		display: flex;
		align-items: center;
		justify-content: center;
	}

	.img {
		width: 56rpx;
		height: 56rpx;
	}
</style>
