<template>
	<view class="wrap">
		<view class="Dev-area">
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">温度</view>
					<image class="Dev-logo" src="/static/temp.png" mode=""></image>
				</view>
				<view class="Dev-data">{{temp}} ℃ </view>
			</view>		
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">甲烷</view>
					<image class="Dev-logo" src="/static/CH4.png" mode=""></image>
				</view>
				<view class="Dev-data">{{CH4}} ! </view>
			</view>
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">湿度</view>
					<image class="Dev-logo" src="/static/humi.png" mode=""></image>
				</view>
				<view class="Dev-data">{{humi}} % </view>
			</view>		
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">CO</view>
					<image class="Dev-logo" src="/static/CO.png" mode=""></image>
				</view>
				<view class="Dev-data">{{CO}}! </view>
			</view>
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">光照</view>
					<image class="Dev-logo" src="/static/lux.png" mode=""></image>
				</view>
				<view class="Dev-data">{{lux}} lx </view>
			</view>
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">红外</view>
					<image class="Dev-logo" src="/static/human.png" mode=""></image>
				</view>
				<view class="Dev-data">{{human}} ! </view>
			</view>
			<view class="Dev-cart">
				<view class="">
					你说我能走到对岸吗？
				</view>
			</view>
			<view class="Dev-cart">
				<view class="">
					<view class="Dev-name">台灯</view>
					<image class="Dev-logo" src="/static/LED.png" mode=""></image>
				</view>
				<switch :checked="LED" @change="onLEDSwitch" color="#2b9939"/> 
			</view>
			<!-- 		<view class="">湿度 {{humi}} % </view>
				<view class="">光照 {{lux}} lx </view>
				<view class="">甲烷 {{CH4}} </view>
				<view class="">一氧化碳 {{CO}} </view>
				<view class="">人体感应 {{human}} </view>
				<switch :checked="LED" @change="onLEDSwitch" /> -->
		</view>
	</view>

</template>

<script>
	const {
		createCommonToken
	} = require('@/key.js')
	export default {
		data() {
			return {
				temp: '',
				humi: '',
				lux: '',
				CH4: false,
				CO: false,
				human: false,
				LED: false,
				token: '',
			}
		},
		onLoad() {
			const params = {
				access_key: 's38Lqymxu6spbc1JQVlfNyVRGe3Lo7O/FX0ttTmnFcU=',
				version: '2022-05-01',
				productid: '6S26Zkf84P',
			}
			this.token = createCommonToken(params)
			//			console.log(this.token)
		},
		onShow() {
			setInterval(() => {
				this.fetchDevData()
			}, 1000)
		},
		methods: {
			fetchDevData() {
				uni.request({
					url: 'https://iot-api.heclouds.com/thingmodel/query-device-property',
					method: 'GET',
					data: {
						product_id: '6S26Zkf84P',
						device_name: 'ESP8266'
					},
					header: {
						'authorization': this.token
					},
					success: (res) => {
						console.log(res.data)
						this.CH4 = res.data.data[0].value
						this.CO = res.data.data[1].value
						this.LED = res.data.data[2].value
						this.human = res.data.data[3].value
						this.humi = res.data.data[4].value
						this.lux = res.data.data[5].value
						this.temp = res.data.data[6].value;
					}
				})

			},
			onLEDSwitch(event) {
				//				console.log(event.detail.value)
				uni.request({
					url: 'https://iot-api.heclouds.com/thingmodel/set-device-property',
					method: 'POST',
					data: {
						product_id: '6S26Zkf84P',
						device_name: 'ESP8266',
						params: {
							"LED": event.detail.value
						}
					},
					header: {
						'authorization': this.token
					},
					success: () => {

					}
				})

			}
		}
	}
</script>

<style>
	.wrap{
		padding: 30rpx;
	}
	.Dev-area {
		display: flex;
		justify-content: space-between;
		flex-wrap: wrap;
	}

	.Dev-cart {
		height: 150rpx;
		width: 320rpx;
		border-radius: 30rpx;
		margin-top: 30rpx;
		display: flex;
		justify-content: space-around;
		align-items: center;
		box-shadow: 0 0 15rpx #ccc;
	}

	.Dev-name {
		font-size: 20rpx;
		text-align: center;
		color: #6d6d6d;
	}

	.Dev-logo {
		height: 70rpx;
		width: 70rpx;
		margin-top: 10rpx;
	}

	.Dev-data {
		font-size: 50rpx;
		;
		color: #6d6d6d;
	}

	.title {
		font-size: 36rpx;
		color: #8f8f94;
	}
</style>