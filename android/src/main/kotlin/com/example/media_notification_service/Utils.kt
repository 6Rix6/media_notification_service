package com.example.media_notification_service

import android.graphics.Bitmap
import android.media.session.MediaSession
import android.media.session.PlaybackState
import java.io.ByteArrayOutputStream

fun bitmapToByteArray(bitmap: Bitmap): ByteArray {
    val stream = ByteArrayOutputStream()
    bitmap.compress(Bitmap.CompressFormat.JPEG, 100, stream)
    return stream.toByteArray()
}

fun Int.toPlaybackStateString(): String {
    return when (this) {
        PlaybackState.STATE_NONE -> "STATE_NONE"
        PlaybackState.STATE_STOPPED -> "STATE_STOPPED"
        PlaybackState.STATE_PAUSED -> "STATE_PAUSED"
        PlaybackState.STATE_PLAYING -> "STATE_PLAYING"
        PlaybackState.STATE_FAST_FORWARDING -> "STATE_FAST_FORWARDING"
        PlaybackState.STATE_REWINDING -> "STATE_REWINDING"
        PlaybackState.STATE_BUFFERING -> "STATE_BUFFERING"
        PlaybackState.STATE_ERROR -> "STATE_ERROR"
        PlaybackState.STATE_CONNECTING -> "STATE_CONNECTING"
        PlaybackState.STATE_SKIPPING_TO_PREVIOUS -> "STATE_SKIPPING_TO_PREVIOUS"
        PlaybackState.STATE_SKIPPING_TO_NEXT -> "STATE_SKIPPING_TO_NEXT"
        PlaybackState.STATE_SKIPPING_TO_QUEUE_ITEM -> "STATE_SKIPPING_TO_QUEUE_ITEM"
        else -> "UNKNOWN ($this)"
    }
}

fun MediaSession.QueueItem.toMap(): Map<String, Any?> {
    val desc = description
    return mapOf(
        "title" to desc.title?.toString(),
        "artist" to desc.subtitle?.toString(),
        "albumArt" to desc.iconBitmap?.let { bitmapToByteArray(it) },
        "albumArtUri" to desc.iconUri?.toString()
    )
}