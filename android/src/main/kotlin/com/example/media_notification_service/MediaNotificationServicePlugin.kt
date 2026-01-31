package com.example.media_notification_service

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.provider.Settings
import android.view.KeyEvent
import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.embedding.engine.plugins.activity.ActivityAware
import io.flutter.embedding.engine.plugins.activity.ActivityPluginBinding
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.PluginRegistry

class MediaNotificationServicePlugin: FlutterPlugin, MethodChannel.MethodCallHandler, 
    ActivityAware, PluginRegistry.ActivityResultListener {
    
    private lateinit var channel: MethodChannel
    private lateinit var eventChannel: EventChannel
    private lateinit var positionEventChannel: EventChannel
    private var context: Context? = null
    private var activityBinding: ActivityPluginBinding? = null
    private var pendingSettingsResult: MethodChannel.Result? = null
    
    private var eventSink: EventChannel.EventSink? = null
    private var positionEventSink: EventChannel.EventSink? = null

    companion object {
        private const val CHANNEL = "com.example.media_notification_service/media"
        private const val EVENT_CHANNEL = "com.example.media_notification_service/media_stream"
        private const val POSITION_EVENT_CHANNEL = "com.example.media_notification_service/position_stream"
        private const val SETTINGS_REQUEST_CODE = 1001
    }

    override fun onAttachedToEngine(flutterPluginBinding: FlutterPlugin.FlutterPluginBinding) {
        context = flutterPluginBinding.applicationContext
        
        channel = MethodChannel(flutterPluginBinding.binaryMessenger, CHANNEL)
        channel.setMethodCallHandler(this)
        
        eventChannel = EventChannel(flutterPluginBinding.binaryMessenger, EVENT_CHANNEL)
        eventChannel.setStreamHandler(object : EventChannel.StreamHandler {
            override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
                eventSink = events
                MediaNotificationListener.setCallback { mediaInfo ->
                    eventSink?.success(mediaInfo)
                }
            }

            override fun onCancel(arguments: Any?) {
                eventSink = null
                MediaNotificationListener.setCallback(null)
            }
        })
        
        positionEventChannel = EventChannel(flutterPluginBinding.binaryMessenger, POSITION_EVENT_CHANNEL)
        positionEventChannel.setStreamHandler(object : EventChannel.StreamHandler {
            override fun onListen(arguments: Any?, events: EventChannel.EventSink?) {
                positionEventSink = events
                MediaNotificationListener.setPositionCallback { positionInfo ->
                    positionEventSink?.success(positionInfo)
                }
            }

            override fun onCancel(arguments: Any?) {
                positionEventSink = null
                MediaNotificationListener.setPositionCallback(null)
            }
        })
    }

    override fun onMethodCall(call: MethodCall, result: MethodChannel.Result) {
        when (call.method) {
            "getCurrentMedia" -> {
                val mediaInfo = getCurrentMediaInfo()
                result.success(mediaInfo)
            }
            "hasPermission" -> {
                result.success(hasNotificationPermission())
            }
            "openSettings" -> {
                pendingSettingsResult = result
                openNotificationSettings()
            }
            "playPause" -> {
                val success = sendMediaAction(KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE)
                result.success(success)
            }
            "skipToNext" -> {
                val success = sendMediaAction(KeyEvent.KEYCODE_MEDIA_NEXT)
                result.success(success)
            }
            "skipToPrevious" -> {
                val success = sendMediaAction(KeyEvent.KEYCODE_MEDIA_PREVIOUS)
                result.success(success)
            }
            "stop" -> {
                val success = sendMediaAction(KeyEvent.KEYCODE_MEDIA_STOP)
                result.success(success)
            }
            "seekTo" -> {
                val position = call.argument<Number>("position")
                if (position != null) {
                    val positionMs = position.toLong()
                    val success = seekTo(positionMs)
                    result.success(success)
                } else {
                    result.error("INVALID_ARGUMENT", "Position must not be null", null)
                }
            }
            else -> result.notImplemented()
        }
    }

    private fun getCurrentMediaInfo(): Map<String, Any?>? {
        val ctx = context ?: return null
        
        try {
            val mediaSessionManager = ctx.getSystemService(Context.MEDIA_SESSION_SERVICE)
                    as MediaSessionManager

            val componentName = ComponentName(ctx, MediaNotificationListener::class.java)
            val controllers = mediaSessionManager.getActiveSessions(componentName)

            if (controllers.isNotEmpty()) {
                val controller = controllers[0]
                val metadata = controller.metadata
                val playbackState = controller.playbackState

                val isPlaying = playbackState?.state == PlaybackState.STATE_PLAYING

                if (playbackState?.state != PlaybackState.STATE_NONE) {
                    val albumArt = metadata?.getBitmap(android.media.MediaMetadata.METADATA_KEY_ALBUM_ART)
                        ?: metadata?.getBitmap(android.media.MediaMetadata.METADATA_KEY_ART)

                    val albumArtBytes = albumArt?.let { bitmapToByteArray(it) }

                    return mapOf(
                        "title" to metadata?.getString(android.media.MediaMetadata.METADATA_KEY_TITLE),
                        "artist" to metadata?.getString(android.media.MediaMetadata.METADATA_KEY_ARTIST),
                        "album" to metadata?.getString(android.media.MediaMetadata.METADATA_KEY_ALBUM),
                        "packageName" to controller.packageName,
                        "albumArt" to albumArtBytes,
                        "isPlaying" to isPlaying,
                        "state" to playbackState?.state?.toPlaybackStateString()
                    )
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return null
    }

    private fun sendMediaAction(keyCode: Int): Boolean {
        val ctx = context ?: return false
        
        try {
            val mediaSessionManager = ctx.getSystemService(Context.MEDIA_SESSION_SERVICE)
                    as MediaSessionManager

            val componentName = ComponentName(ctx, MediaNotificationListener::class.java)
            val controllers = mediaSessionManager.getActiveSessions(componentName)

            if (controllers.isNotEmpty()) {
                val controller = controllers[0]

                val keyEvent = KeyEvent(KeyEvent.ACTION_DOWN, keyCode)
                controller.dispatchMediaButtonEvent(keyEvent)

                val keyEventUp = KeyEvent(KeyEvent.ACTION_UP, keyCode)
                controller.dispatchMediaButtonEvent(keyEventUp)

                return true
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return false
    }

    private fun seekTo(positionMs: Long): Boolean {
        val ctx = context ?: return false
        
        try {
            val mediaSessionManager = ctx.getSystemService(Context.MEDIA_SESSION_SERVICE)
                    as MediaSessionManager

            val componentName = ComponentName(ctx, MediaNotificationListener::class.java)
            val controllers = mediaSessionManager.getActiveSessions(componentName)

            if (controllers.isNotEmpty()) {
                val controller = controllers[0]
                controller.transportControls.seekTo(positionMs)
                return true
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return false
    }

    private fun hasNotificationPermission(): Boolean {
        val ctx = context ?: return false
        
        val enabledListeners = Settings.Secure.getString(
            ctx.contentResolver,
            "enabled_notification_listeners"
        )
        val packageName = ctx.packageName
        return enabledListeners != null && enabledListeners.contains(packageName)
    }

    private fun openNotificationSettings() {
        val activity = activityBinding?.activity ?: return
        
        val intent = Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS)
        activity.startActivityForResult(intent, SETTINGS_REQUEST_CODE)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?): Boolean {
        if (requestCode == SETTINGS_REQUEST_CODE) {
            pendingSettingsResult?.success(null)
            pendingSettingsResult = null
            return true
        }
        return false
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        channel.setMethodCallHandler(null)
        context = null
    }

    override fun onAttachedToActivity(binding: ActivityPluginBinding) {
        activityBinding = binding
        binding.addActivityResultListener(this)
    }

    override fun onDetachedFromActivityForConfigChanges() {
        activityBinding?.removeActivityResultListener(this)
        activityBinding = null
    }

    override fun onReattachedToActivityForConfigChanges(binding: ActivityPluginBinding) {
        activityBinding = binding
        binding.addActivityResultListener(this)
    }

    override fun onDetachedFromActivity() {
        activityBinding?.removeActivityResultListener(this)
        activityBinding = null
    }
}