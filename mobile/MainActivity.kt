package com.faton.console

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.KeyEvent
import android.view.inputmethod.EditorInfo
import android.widget.*
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import java.io.InputStream
import java.io.OutputStream
import java.util.*

class MainActivity : AppCompatActivity() {

    private lateinit var tvTerminal: TextView
    private lateinit var etInput: EditText
    private lateinit var scrollTerminal: ScrollView

    private var bluetoothAdapter: BluetoothAdapter? = null
    private var bluetoothSocket: BluetoothSocket? = null
    private var outputStream: OutputStream? = null
    private var inputStream: InputStream? = null
    private var connectedThread: ConnectedThread? = null

    private val handler = Handler(Looper.getMainLooper())
    private val uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        tvTerminal = findViewById(R.id.tvTerminal)
        etInput = findViewById(R.id.etInput)
        scrollTerminal = findViewById(R.id.scrollTerminal)

        bluetoothAdapter = BluetoothAdapter.getDefaultAdapter()

        if (bluetoothAdapter == null) {
            append("[!] NO BLUETOOTH\n")
            return
        }

        // Автоподключение
        if (!bluetoothAdapter!!.isEnabled) {
            append("[!] ENABLE BT FIRST\n")
        } else {
            connect()
        }

        // Отправка по Enter
        etInput.setOnEditorActionListener { _, actionId, event ->
            if (actionId == EditorInfo.IME_ACTION_SEND ||
                (event != null && event.keyCode == KeyEvent.KEYCODE_ENTER && event.action == KeyEvent.ACTION_DOWN)) {
                sendCommand()
                return@setOnEditorActionListener true
            }
            false
        }
    }

    private fun connect() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT)
                != PackageManager.PERMISSION_GRANTED
            ) {
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(Manifest.permission.BLUETOOTH_CONNECT, Manifest.permission.BLUETOOTH_SCAN),
                    1
                )
                return
            }
        }

        append("[*] SCANNING FOR FATONOS...\n")

        val paired = bluetoothAdapter!!.bondedDevices
        var device: BluetoothDevice? = null

        for (d in paired) {
            if (d.name == "FatonOS") {
                device = d
                break
            }
        }

        if (device == null) {
            append("[!] FATONOS NOT PAIRED\n")
            append("[!] PAIR IN SETTINGS FIRST\n")
            return
        }

        try {
            bluetoothSocket = device.createRfcommSocketToServiceRecord(uuid)
            bluetoothAdapter?.cancelDiscovery()
            bluetoothSocket?.connect()

            outputStream = bluetoothSocket?.outputStream
            inputStream = bluetoothSocket?.inputStream

            connectedThread = ConnectedThread()
            connectedThread?.start()

            append("[+] CONNECTED\n\n")

        } catch (e: Exception) {
            append("[!] CONN FAIL: ${e.message}\n")
        }
    }

    private fun sendCommand() {
        val cmd = etInput.text.toString()
        if (cmd.isEmpty()) return

        append("> $cmd\n")

        try {
            outputStream?.write("$cmd\r\n".toByteArray())
            outputStream?.flush()
        } catch (e: Exception) {
            append("[!] TX FAIL\n")
        }

        etInput.text.clear()
    }

    private fun append(text: String) {
        handler.post {
            tvTerminal.append(text)
            scrollTerminal.post {
                scrollTerminal.fullScroll(android.view.View.FOCUS_DOWN)
            }
        }
    }

    private inner class ConnectedThread : Thread() {
        override fun run() {
            val buf = ByteArray(1024)
            var bytes: Int

            while (!isInterrupted) {
                try {
                    bytes = inputStream?.read(buf) ?: -1
                    if (bytes == -1) break

                    val text = String(buf, 0, bytes, Charsets.UTF_8)
                    append(text)

                } catch (e: Exception) {
                    append("\n[!] LINK DEAD\n")
                    break
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        try {
            connectedThread?.interrupt()
            inputStream?.close()
            outputStream?.close()
            bluetoothSocket?.close()
        } catch (_: Exception) {}
    }
}