package com.timespeed.lib

import android.content.Context
import android.os.SystemClock

object TimeSpeed {
    init {
        System.loadLibrary("substrate")
        System.loadLibrary("time-speed")
    }

    /**
     * 加速正常时间
     */
    private external fun speedUTCTime(substratePath: String, velocity: Float)

    /**
     * 加速开机时间
     */
    private external fun speedClockTime(substratePath: String, velocity: Float)


    fun speedUTCTime(context: Context, velocity: Float) {
        val path = context.applicationInfo.nativeLibraryDir + "/libsubstrate.so"
        speedUTCTime(path, velocity)
    }

    fun speedClockTime(context: Context, velocity: Float) {
        val path = context.applicationInfo.nativeLibraryDir + "/libsubstrate.so"
        speedClockTime(path, velocity)
    }

    external fun getRealClockTime(): Long
    external fun getRealUTCTime(): Long
}