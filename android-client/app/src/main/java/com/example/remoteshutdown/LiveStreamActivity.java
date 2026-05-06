/*Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
package com.example.remoteshutdown;

import android.os.Bundle;
import android.view.View;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import org.videolan.libvlc.LibVLC;
import org.videolan.libvlc.Media;
import org.videolan.libvlc.MediaPlayer;
import org.videolan.libvlc.util.VLCVideoLayout;
import java.util.ArrayList;

public class LiveStreamActivity extends AppCompatActivity {

    private LibVLC libvlc;
    private MediaPlayer mediaplayer;
    private VLCVideoLayout videolayout;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN, android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_live_stream);

        videolayout = findViewById(R.id.video_layout);

        try {
            ArrayList<String> res = new ArrayList<>();
            res.add("--no-drop-late-frames");
            res.add("--no-skip-frames");
            res.add("--rtsp-tcp");
            res.add("-vvv");
            res.add("--network-caching=150");
            res.add("--clock-jitter=0");
            res.add("--live-caching=150");

            libvlc = new LibVLC(this, res);
            mediaplayer = new MediaPlayer(libvlc);
            mediaplayer.attachViews(videolayout, null, false, false);

            Media ans = new Media(libvlc, android.net.Uri.parse("udp://@:1234"));
            ans.setHWDecoderEnabled(true, false);
            ans.addOption(":network-caching=150");
            ans.addOption(":clock-jitter=0");
            ans.addOption(":live-caching=150");

            mediaplayer.setMedia(ans);
            ans.release();
            mediaplayer.play();

            Toast.makeText(this, "🔴 Đang nhận Stream UDP...", Toast.LENGTH_SHORT).show();
        } catch (Exception e) {
            Toast.makeText(this, "Lỗi khởi tạo VLC: " + e.getMessage(), Toast.LENGTH_LONG).show();
        }

        View btnBack = findViewById(R.id.btn_stream_back);
        if (btnBack != null) {
            btnBack.setOnClickListener(v -> finish());
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        if (mediaplayer != null) {
            mediaplayer.stop();
            mediaplayer.detachViews();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (mediaplayer != null) {
            mediaplayer.release();
            mediaplayer = null;
        }
        if (libvlc != null) {
            libvlc.release();
            libvlc = null;
        }
    }
}
