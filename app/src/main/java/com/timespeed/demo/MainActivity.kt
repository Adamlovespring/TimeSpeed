package com.timespeed.demo

import android.os.Bundle
import android.os.SystemClock
import androidx.appcompat.app.AppCompatActivity
import com.timespeed.lib.TimeSpeed
import kotlinx.android.synthetic.main.activity_main.*
import java.util.*

class MainActivity : AppCompatActivity() {

    private val timer: Timer = Timer()
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        TimeSpeed.speedClockTime(this, 10f)

        val realTimePlace = getString(R.string.real_time)
        val speedTimePlace = getString(R.string.speed_time)

        val postDelayed = {
            speed_time_text.text =
                String.format(speedTimePlace, SystemClock.uptimeMillis().toString())
            real_time_text.text =
                String.format(realTimePlace, TimeSpeed.getRealClockTime().toString())
        }
        timer.schedule(object : TimerTask() {
            override fun run() {
                speed_time_text.post(postDelayed)
            }
        }, 1000, 500)
    }

    override fun onDestroy() {
        super.onDestroy()
        timer.cancel()
    }
}
