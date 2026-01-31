package com.example.media_notification_service

import android.content.ComponentName
import android.media.MediaMetadata
import android.media.session.MediaController
import android.media.session.MediaSession
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.os.Handler
import android.os.Looper
import android.service.notification.NotificationListenerService

class MediaNotificationListener : NotificationListenerService() {

    companion object {
        private var callback: ((Map<String, Any?>?) -> Unit)? = null
        private var positionCallback: ((Map<String, Any?>?) -> Unit)? = null

        fun setCallback(cb: ((Map<String, Any?>?) -> Unit)?) {
            callback = cb
        }

        fun setPositionCallback(cb: ((Map<String, Any?>?) -> Unit)?) {
            positionCallback = cb
        }
    }

    private var mediaSessionManager: MediaSessionManager? = null
    private val mediaControllerCallbacks = mutableMapOf<MediaController, MediaController.Callback>()
    private var currentController: MediaController? = null

    // Position更新用
    private val positionHandler = Handler(Looper.getMainLooper())
    private var positionUpdateRunnable: Runnable? = null
    private val POSITION_UPDATE_INTERVAL = 100L // 100ms間隔で更新

    override fun onListenerConnected() {
        super.onListenerConnected()

        mediaSessionManager = getSystemService(MEDIA_SESSION_SERVICE) as MediaSessionManager

        val componentName = ComponentName(this, MediaNotificationListener::class.java)
        mediaSessionManager?.addOnActiveSessionsChangedListener(
            { controllers ->
                updateMediaControllers(controllers)
            },
            componentName
        )

        val controllers = mediaSessionManager?.getActiveSessions(componentName)
        controllers?.let { updateMediaControllers(it) }
    }

    private fun updateMediaControllers(controllers: List<MediaController>?) {
        mediaControllerCallbacks.forEach { (controller, callback) ->
            controller.unregisterCallback(callback)
        }
        mediaControllerCallbacks.clear()

        currentController = controllers?.firstOrNull()

        controllers?.forEach { controller ->
            val callback = object : MediaController.Callback() {
                override fun onMetadataChanged(metadata: MediaMetadata?) {
                    notifyMediaChange(controller, metadata, controller.playbackState,
                        songChanged = true,
                        queueChanged = true
                    )
                }

                override fun onPlaybackStateChanged(state: PlaybackState?) {
                    notifyMediaChange(controller, controller.metadata, state,
                        songChanged = false,
                        queueChanged = false
                    )

                    // 再生状態が変わったらPosition更新を開始/停止
                    if (state?.state == PlaybackState.STATE_PLAYING) {
                        startPositionUpdates(controller)
                    } else {
                        stopPositionUpdates()
                        // 停止時も現在の位置を送信
                        notifyPositionChange(controller, state)
                    }
                }

                override fun onQueueChanged(queue: List<MediaSession.QueueItem?>?) {
                    notifyMediaChange(controller,controller.metadata,controller.playbackState,
                        songChanged = false,
                        queueChanged = true
                    )
                }
            }

            controller.registerCallback(callback)
            mediaControllerCallbacks[controller] = callback

            // 初期状態でも再生中ならPosition更新を開始
            if (controller.playbackState?.state == PlaybackState.STATE_PLAYING) {
                startPositionUpdates(controller)
            }
        }
    }

    private fun startPositionUpdates(controller: MediaController) {
        stopPositionUpdates()

        positionUpdateRunnable = object : Runnable {
            override fun run() {
                notifyPositionChange(controller, controller.playbackState)
                positionHandler.postDelayed(this, POSITION_UPDATE_INTERVAL)
            }
        }

        positionHandler.post(positionUpdateRunnable!!)
    }

    private fun stopPositionUpdates() {
        positionUpdateRunnable?.let {
            positionHandler.removeCallbacks(it)
        }
        positionUpdateRunnable = null
    }

    private fun notifyPositionChange(controller: MediaController, state: PlaybackState?) {
        if (state == null) {
            positionCallback?.invoke(null)
            return
        }

        val position = state.position
        val duration = controller.metadata?.getLong(MediaMetadata.METADATA_KEY_DURATION) ?: 0L
        val playbackSpeed = state.playbackSpeed

        positionCallback?.invoke(mapOf(
            "position" to position,
            "duration" to duration,
            "playbackSpeed" to playbackSpeed,
            "state" to state.state.toPlaybackStateString()
        ))
    }

    private fun notifyMediaChange(
        controller: MediaController,
        metadata: MediaMetadata?,
        state: PlaybackState?,
        songChanged: Boolean,
        queueChanged: Boolean
    ) {
        val isMediaExisting = state?.state != PlaybackState.STATE_NONE
        val isPlaying = state?.state == PlaybackState.STATE_PLAYING

        if (!isMediaExisting) {
            callback?.invoke(null)
            return
        }

        val albumArt = metadata?.getBitmap(MediaMetadata.METADATA_KEY_ALBUM_ART)
            ?: metadata?.getBitmap(MediaMetadata.METADATA_KEY_ART)

        val albumArtBytes = albumArt?.let { bitmapToByteArray(it) }

        val currentQueueIndex = controller.queue?.indexOfFirst {
            it.queueId == controller.playbackState?.activeQueueItemId
        } ?: -1

        val queue = controller.queue ?: emptyList()

        val nextItem = queue.getOrNull(currentQueueIndex + 1)?.toMap() ?: emptyMap()
        val prevItem = queue.getOrNull(currentQueueIndex - 1)?.toMap() ?: emptyMap()


        callback?.invoke(mapOf(
            "title" to metadata?.getString(MediaMetadata.METADATA_KEY_TITLE),
            "artist" to metadata?.getString(MediaMetadata.METADATA_KEY_ARTIST),
            "album" to metadata?.getString(MediaMetadata.METADATA_KEY_ALBUM),
            "packageName" to controller.packageName,
            "albumArt" to albumArtBytes,
            "isPlaying" to isPlaying,
            "state" to state?.state?.toPlaybackStateString(),
            "songChanged" to songChanged,
            "queueChanged" to queueChanged,
            "nextItem" to nextItem,
            "previousItem" to prevItem,
        ))
    }

    override fun onListenerDisconnected() {
        stopPositionUpdates()
        mediaControllerCallbacks.forEach { (controller, callback) ->
            controller.unregisterCallback(callback)
        }
        mediaControllerCallbacks.clear()
        super.onListenerDisconnected()
    }
}