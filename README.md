# 说明
在获取时间时，通常使用`System.currentTimeMillis()`获取系统时间，该时间是我们生活中使用的时间，即UTC时间，该时间可通过设置应用进行修改，正因为如此，我们在使用该时间作为计量单位时，可能出现真实时间被篡改所带来的一些不确定问题。另外一种则是系统开机时间，可以使用`SystemClock.uptimeMillis()`方法获取，该时间无法修改，但每次重启设备都会被重置。

在Android的动画系统或者游戏的动画系统中，通常使用开机时间来计算动画播放速度，诸如回合制游戏，基本都是根据数据按时间播放一段动画完成游戏过程，这类游戏耗费大量时间，从而市面上有人开发游戏加速助手通过加速时间来使真个过程变快。

众所周知，Android内核是基于Linux的，那么Android中获取时间的方法，本质是调用`/system/lib/libc.so`中的`gettimeofday`方法和`clock_gettime`方法，前者获取utc时间，后者获取开机时间，通过hook这两个函数，则可以改变当前进程获取的时间。

# 使用
该项目支持对这两种时间的速度改变，加快或减慢，示例代码如下

``` kotlin
TimeSpeed.speedClockTime(this, 10f)  //加快开机时间
TimeSpeed.speedClockTime(this, 0.5f)  //减慢开机时间
```

``` kotlin
TimeSpeed.speedUTCTime(this, 10f)  //加快UTC时间
TimeSpeed.speedUTCTime(this, 0.5f)  //减慢UTC时间
```

时间加快后如果想停下来，可以把速率传1，但时间不可倒退，即加速的时间不能变成正常时间，由于加速是以进程为单位的，所以只会改变有调用该方法进程的时间，不会影响其他进程或其他App。

