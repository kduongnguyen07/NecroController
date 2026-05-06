/* Author : Necrosid3 - Duong Nguyen Khanh - UET_VNU */
package com.example.remoteshutdown;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Typeface;
import android.net.Uri;
import android.os.Bundle;
import android.speech.RecognizerIntent;
import android.util.Base64;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.LinearLayout;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Locale;

public class ControlActivity extends AppCompatActivity {

    private String targetip;
    private String targetmac;
    private android.app.ProgressDialog webcamProgress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN, android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_control);

        targetip = getIntent().getStringExtra("IP_ADDRESS");
        targetmac = getIntent().getStringExtra("MAC_ADDRESS");
        String devicename = getIntent().getStringExtra("DEVICE_NAME");

        try {
            TextView tvName = findViewById(R.id.tv_device_name);
            if (tvName != null) tvName.setText(devicename);
        } catch (Exception e) {}

        try {
            TextView tvIp = findViewById(R.id.tv_device_ip);
            if (tvIp != null) tvIp.setText(targetip);
        } catch (Exception e) {}

        SharedPreferences prefs = getSharedPreferences("AppConfig", MODE_PRIVATE);
        String savedBg = prefs.getString("bg_uri", null);
        String savedFont = prefs.getString("font_type", "SANS_SERIF");

        if (savedBg != null) {
            try {
                ImageView imgBg = findViewById(R.id.img_control_bg);
                if (imgBg != null) {
                    Uri res = Uri.parse(savedBg);
                    InputStream stream = getContentResolver().openInputStream(res);
                    if (stream != null) {
                        Bitmap ans = BitmapFactory.decodeStream(stream);
                        imgBg.setImageBitmap(ans);
                        stream.close();
                    }
                }
            } catch (Exception e) {
                prefs.edit().remove("bg_uri").apply();
            }
        }

        try {
            findViewById(android.R.id.content).postDelayed(() -> applyFontToView(getWindow().getDecorView().getRootView(), getFontType(savedFont)), 100);
        } catch (Exception e) {}

        try {
            SeekBar sbVolume = findViewById(R.id.sb_volume);
            TextView tvVolPreview = findViewById(R.id.tv_vol_preview);

            if (sbVolume != null) {
                sbVolume.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
                    @Override
                    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                        if (tvVolPreview != null) {
                            tvVolPreview.setText("ÂM LƯỢNG MÁY TÍNH: " + progress + "%");
                        }
                    }
                    @Override
                    public void onStartTrackingTouch(SeekBar seekBar) {}

                    @Override
                    public void onStopTrackingTouch(SeekBar seekBar) {
                        int res = seekBar.getProgress();
                        send_socket_command("VOL_SET_" + res);
                    }
                });
            }
        } catch (Exception e) {}

        safeSetClickListener(R.id.btn_off, "OFF");
        safeSetClickListener(R.id.btn_lock, "LOCK");
        safeSetClickListener(R.id.btn_sleep, "SLEEP");
        safeSetClickListener(R.id.btn_abort, "ABORT");
        safeSetClickListener(R.id.btn_wake, "RESTART");

        try {
            View btnTimer = findViewById(R.id.btn_timer);
            if (btnTimer != null) btnTimer.setOnClickListener(v -> show_timer_dialog());
        } catch (Exception e) {}

        try {
            View btnMonitor = findViewById(R.id.btn_open_monitor);
            if (btnMonitor != null) {
                btnMonitor.setOnClickListener(v -> {
                    android.content.Intent ans = new android.content.Intent(ControlActivity.this, MonitorActivity.class);
                    ans.putExtra("IP_ADDRESS", targetip);
                    startActivity(ans);
                });
            }
        } catch (Exception e) {}

        try {
            View btnCam = findViewById(R.id.btn_stealth_webcam);
            if (btnCam != null) {
                btnCam.setOnClickListener(v -> {
                    show_loading("Đang gọi Webcam, ráng đợi...");
                    send_socket_command("WEBCAM");
                });
            }
        } catch (Exception e) {}

        try {
            View btnScreenshot = findViewById(R.id.btn_screenshot);
            if (btnScreenshot != null) {
                btnScreenshot.setOnClickListener(v -> {
                    show_loading("Đang chụp màn hình...");
                    send_socket_command("SCREENSHOT");
                });
            }
        } catch (Exception e) {}

        try {
            View btnShell = findViewById(R.id.btn_shell);
            if (btnShell != null) {
                btnShell.setOnClickListener(v -> show_shell_dialog());
            }
        } catch (Exception e) {}

        try {
            View btnVoice = findViewById(R.id.btn_voice_control);
            if (btnVoice != null) {
                btnVoice.setOnClickListener(v -> {
                    Intent res = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
                    res.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
                    res.putExtra(RecognizerIntent.EXTRA_LANGUAGE, "vi-VN");
                    res.putExtra(RecognizerIntent.EXTRA_PROMPT, "Nói lệnh đi boss...");
                    try {
                        startActivityForResult(res, 300);
                    } catch (Exception ex) {
                        Toast.makeText(this, "Điện thoại đéo hỗ trợ giọng nói!", Toast.LENGTH_SHORT).show();
                    }
                });
            }
        } catch (Exception e) {}

        try {
            View btnClip = findViewById(R.id.btn_clipboard_sync);
            if (btnClip != null) {
                btnClip.setOnClickListener(v -> {
                    show_loading("Đang đồng bộ Clipboard...");
                    send_socket_clipboard_command("GET_CLIPBOARD");
                });
            }
        } catch (Exception e) {}

        try {
            View btnStream = findViewById(R.id.btn_live_stream);
            if (btnStream != null) {
                btnStream.setOnClickListener(v -> {
                    show_loading("Đang khởi động Live Stream...");
                    start_live_stream();
                });
            }
        } catch (Exception e) {}

        try {
            View btnSearch = findViewById(R.id.btn_file_search);
            if (btnSearch != null) {
                btnSearch.setOnClickListener(v -> show_file_search_dialog());
            }
        } catch (Exception e) {}

        try {
            View btnDownload = findViewById(R.id.btn_file_download);
            if (btnDownload != null) {
                btnDownload.setOnClickListener(v -> show_file_download_dialog());
            }
        } catch (Exception e) {}
    }

    private void safeSetClickListener(int id, String command) {
        try {
            View btn = findViewById(id);
            if (btn != null) {
                btn.setOnClickListener(v -> {
                    send_socket_command(command);
                });
            }
        } catch (Exception e) {}
    }

    private void show_loading(String message) {
        if (webcamProgress == null) {
            webcamProgress = new android.app.ProgressDialog(this);
            webcamProgress.setCancelable(false);
        }
        webcamProgress.setMessage(message);
        webcamProgress.show();

        new android.os.Handler(android.os.Looper.getMainLooper()).postDelayed(() -> {
            if (webcamProgress != null && webcamProgress.isShowing()) {
                webcamProgress.dismiss();
                Toast.makeText(ControlActivity.this, "Timeout! Server không phản hồi.", Toast.LENGTH_SHORT).show();
            }
        }, 15000);
    }

    private void dismiss_loading() {
        if (webcamProgress != null && webcamProgress.isShowing()) {
            webcamProgress.dismiss();
        }
    }

    private void show_timer_dialog() {
        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
        builder.setTitle("Hẹn giờ (Phút)");
        final android.widget.EditText input = new android.widget.EditText(this);
        input.setInputType(android.text.InputType.TYPE_CLASS_NUMBER);
        builder.setView(input);
        builder.setPositiveButton("OK", (dialog, which) -> {
            String ans = input.getText().toString();
            if (!ans.isEmpty()) {
                int res = Integer.parseInt(ans) * 60;
                send_socket_command("TIMER_" + res);
            }
        });
        builder.show();
    }

    private void show_shell_dialog() {
        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
        builder.setTitle("Chạy lệnh Shell trên PC");
        final android.widget.EditText input = new android.widget.EditText(this);
        input.setHint("VD: ipconfig, tasklist, dir...");
        builder.setView(input);
        builder.setPositiveButton("CHẠY", (dialog, which) -> {
            String ans = input.getText().toString().trim();
            if (!ans.isEmpty()) {
                show_loading("Đang chạy lệnh Shell...");
                send_socket_read_command("CMD_SHELL_" + ans);
            }
        });
        builder.setNegativeButton("Huỷ", null);
        builder.show();
    }

    private void show_file_search_dialog() {
        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
        builder.setTitle("Tìm kiếm File (Everything-style)");
        final android.widget.EditText input = new android.widget.EditText(this);
        input.setHint("Tên file cần tìm...");
        builder.setView(input);
        builder.setPositiveButton("TÌM", (dialog, which) -> {
            String ans = input.getText().toString().trim();
            if (!ans.isEmpty()) {
                show_loading("Đang quét toàn bộ file...");
                send_socket_read_command("CMD_SEARCH_" + ans);
            }
        });
        builder.setNegativeButton("Huỷ", null);
        builder.show();
    }

    private void show_file_download_dialog() {
        android.app.AlertDialog.Builder builder = new android.app.AlertDialog.Builder(this);
        builder.setTitle("Tải File từ PC");
        final android.widget.EditText input = new android.widget.EditText(this);
        input.setHint("Đường dẫn file trên PC...");
        builder.setView(input);
        builder.setPositiveButton("TẢI", (dialog, which) -> {
            String ans = input.getText().toString().trim();
            if (!ans.isEmpty()) {
                show_loading("Đang tải file về...");
                download_file_from_server(ans);
            }
        });
        builder.setNegativeButton("Huỷ", null);
        builder.show();
    }

    private void download_file_from_server(String remotePath) {
        new Thread(() -> {
            String resultMsg = null;
            try (Socket socket = new Socket(targetip, 9999)) {
                socket.setSoTimeout(60000);
                java.io.OutputStream out = socket.getOutputStream();
                java.io.InputStream in = socket.getInputStream();

                String res = "FILE_DOWNLOAD_" + remotePath + "\n";
                out.write(res.getBytes());
                out.flush();

                StringBuilder header = new StringBuilder();
                int ans;
                while ((ans = in.read()) != -1) {
                    if (ans == '\n') break;
                    header.append((char) ans);
                }

                String headerStr = header.toString().trim();
                if (headerStr.startsWith("FILE_BIN_")) {
                    long fileSize = Long.parseLong(headerStr.substring(9));

                    String fileName = remotePath;
                    int lastSlash = Math.max(remotePath.lastIndexOf('\\'), remotePath.lastIndexOf('/'));
                    if (lastSlash >= 0 && lastSlash < remotePath.length() - 1) {
                        fileName = remotePath.substring(lastSlash + 1);
                    }

                    android.content.ContentValues values = new android.content.ContentValues();
                    values.put(android.provider.MediaStore.Downloads.DISPLAY_NAME, fileName);
                    values.put(android.provider.MediaStore.Downloads.RELATIVE_PATH,
                            android.os.Environment.DIRECTORY_DOWNLOADS + "/NeroController");

                    android.net.Uri uri = getContentResolver().insert(
                            android.provider.MediaStore.Downloads.EXTERNAL_CONTENT_URI, values);

                    if (uri != null) {
                        try (java.io.OutputStream fileOut = getContentResolver().openOutputStream(uri)) {
                            byte[] buf = new byte[8192];
                            long totalRead = 0;
                            while (totalRead < fileSize) {
                                int toRead = (int) Math.min(buf.length, fileSize - totalRead);
                                int bytesRead = in.read(buf, 0, toRead);
                                if (bytesRead <= 0) break;
                                fileOut.write(buf, 0, bytesRead);
                                totalRead += bytesRead;
                            }
                        }
                        resultMsg = "✅ File saved to: Downloads/NeroController/" + fileName;
                    } else {
                        resultMsg = "❌ Không tạo được file trong Downloads!";
                    }
                } else if (headerStr.startsWith("ERROR_")) {
                    resultMsg = "❌ Server: " + headerStr;
                } else {
                    resultMsg = "❌ Phản hồi lạ: " + headerStr;
                }
            } catch (Exception e) {
                resultMsg = "❌ Lỗi: " + e.getMessage();
            }

            String finalMsg = resultMsg;
            runOnUiThread(() -> {
                dismiss_loading();
                Toast.makeText(this, finalMsg, Toast.LENGTH_LONG).show();
            });
        }).start();
    }

    private void send_socket_read_command(String command) {
        new Thread(() -> {
            String resData = null;
            try (Socket socket = new Socket(targetip, 9999);
                 PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

                socket.setSoTimeout(60000);
                out.println(command);

                StringBuilder res = new StringBuilder();
                String line;
                while ((line = in.readLine()) != null) {
                    res.append(line).append("\n");
                }
                resData = res.toString();
            } catch (Exception e) {
                resData = "LỖI: " + e.getMessage();
            }

            String finalData = resData;
            runOnUiThread(() -> {
                dismiss_loading();
                show_result_dialog(command.startsWith("CMD_SEARCH_") ? "Kết quả tìm kiếm" : "Kết quả Shell", finalData);
            });
        }).start();
    }

    private void show_result_dialog(String title, String content) {
        boolean isSearch = title.contains("tìm kiếm");

        if (isSearch && content != null && !content.isEmpty() && !content.equals("NOT_FOUND\n")) {
            String[] lines = content.split("\n");

            LinearLayout container = new LinearLayout(this);
            container.setOrientation(LinearLayout.VERTICAL);
            container.setPadding(20, 10, 20, 10);

            TextView tvCount = new TextView(this);
            tvCount.setText("📁 Tìm thấy " + lines.length + " file:");
            tvCount.setTextColor(0xFF03DAC5);
            tvCount.setTextSize(14);
            tvCount.setTypeface(Typeface.DEFAULT_BOLD);
            tvCount.setPadding(10, 10, 10, 20);
            container.addView(tvCount);

            for (String line : lines) {
                final String filePath = line.trim();
                if (filePath.isEmpty()) continue;

                LinearLayout item = new LinearLayout(this);
                item.setOrientation(LinearLayout.HORIZONTAL);
                item.setPadding(16, 14, 16, 14);
                item.setGravity(android.view.Gravity.CENTER_VERTICAL);
                item.setBackground(getResources().getDrawable(android.R.drawable.list_selector_background));

                String fileName = filePath;
                int lastSlash = Math.max(filePath.lastIndexOf('\\'), filePath.lastIndexOf('/'));
                if (lastSlash >= 0) fileName = filePath.substring(lastSlash + 1);

                TextView tvName = new TextView(this);
                tvName.setText("📄 " + fileName);
                tvName.setTextColor(0xFFFFFFFF);
                tvName.setTextSize(13);
                tvName.setSingleLine(true);
                tvName.setEllipsize(android.text.TextUtils.TruncateAt.MIDDLE);
                tvName.setLayoutParams(new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));
                item.addView(tvName);

                TextView tvDl = new TextView(this);
                tvDl.setText("⬇");
                tvDl.setTextSize(18);
                tvDl.setPadding(16, 0, 0, 0);
                item.addView(tvDl);

                item.setOnClickListener(v -> {
                    show_loading("Đang tải");
                    download_file_from_server(filePath);
                });

                container.addView(item);

                View divider = new View(this);
                divider.setLayoutParams(new LinearLayout.LayoutParams(
                        LinearLayout.LayoutParams.MATCH_PARENT, 1));
                divider.setBackgroundColor(0x33FFFFFF);
                container.addView(divider);
            }

            android.widget.ScrollView scrollView = new android.widget.ScrollView(this);
            scrollView.addView(container);

            new android.app.AlertDialog.Builder(this)
                    .setTitle(title)
                    .setView(scrollView)
                    .setPositiveButton("ĐÓNG", null)
                    .show();
        } else {
            android.widget.ScrollView scrollView = new android.widget.ScrollView(this);
            TextView tv = new TextView(this);
            tv.setText(content != null && !content.isEmpty() ? content : "Không có kết quả.");
            tv.setTextColor(0xFFFFFFFF);
            tv.setTextSize(12);
            tv.setPadding(30, 20, 30, 20);
            tv.setTypeface(Typeface.MONOSPACE);
            scrollView.addView(tv);

            new android.app.AlertDialog.Builder(this)
                    .setTitle(title)
                    .setView(scrollView)
                    .setPositiveButton("OK", null)
                    .show();
        }
    }

    private void send_socket_command(String command) {
        new Thread(() -> {
            boolean isSuccess = false;
            String resData = null;
            try (Socket socket = new Socket(targetip, 9999);
                 PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

                if (command.equals("WEBCAM") || command.equals("SCREENSHOT")) {
                    socket.setSoTimeout(15000);
                }

                out.println(command);

                if (command.equals("WEBCAM") || command.equals("SCREENSHOT")) {
                    resData = in.readLine();
                }
                isSuccess = true;
            } catch (Exception e) {
                isSuccess = false;
            }

            boolean finalSuccess = isSuccess;
            String finalData = resData;
            runOnUiThread(() -> {
                dismiss_loading();

                if (finalSuccess) {
                    if (command.equals("WEBCAM") || command.equals("SCREENSHOT")) {
                        if (finalData != null && finalData.startsWith("WEBCAM_DATA_")) {
                            show_stealth_photo(finalData.substring(12));
                        } else {
                            Toast.makeText(this, "Lỗi Camera: " + finalData, Toast.LENGTH_SHORT).show();
                        }
                    } else {
                        Toast.makeText(this, "Đã bắn lệnh: " + command, Toast.LENGTH_SHORT).show();
                    }
                } else {
                    Toast.makeText(this, "Đéo kết nối được Laptop", Toast.LENGTH_SHORT).show();
                }
            });
        }).start();
    }

    private void show_stealth_photo(String base64Str) {
        try {
            byte[] resBytes = Base64.decode(base64Str, Base64.DEFAULT);
            Bitmap ans = BitmapFactory.decodeByteArray(resBytes, 0, resBytes.length);

            android.content.ContentValues values = new android.content.ContentValues();
            values.put(android.provider.MediaStore.Images.Media.DISPLAY_NAME, "NeroCapture_" + System.currentTimeMillis() + ".jpg");
            values.put(android.provider.MediaStore.Images.Media.MIME_TYPE, "image/jpeg");
            values.put(android.provider.MediaStore.Images.Media.RELATIVE_PATH, android.os.Environment.DIRECTORY_PICTURES + "/NeroCaptures");

            android.net.Uri uri = getContentResolver().insert(android.provider.MediaStore.Images.Media.EXTERNAL_CONTENT_URI, values);
            if (uri != null) {
                try (java.io.OutputStream out = getContentResolver().openOutputStream(uri)) {
                    ans.compress(Bitmap.CompressFormat.JPEG, 100, out);
                }
            }

            ImageView imgView = new ImageView(this);
            imgView.setImageBitmap(ans);
            imgView.setPadding(20, 20, 20, 20);

            new android.app.AlertDialog.Builder(this)
                    .setTitle("📸 Mồi đã vào tròng! (Đã lưu Gallery)")
                    .setView(imgView)
                    .setPositiveButton("OK", null)
                    .show();
        } catch (Exception e) {
            Toast.makeText(this, "Ảnh bị lỗi rồi", Toast.LENGTH_SHORT).show();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 300 && resultCode == RESULT_OK && data != null) {
            ArrayList<String> results = data.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);
            if (results != null && !results.isEmpty()) {
                String res = results.get(0).toLowerCase(Locale.ROOT);
                Toast.makeText(this, "🎤 Nhận lệnh: " + res, Toast.LENGTH_SHORT).show();
                send_voice_command(res);
            }
        }
    }

    private void send_voice_command(String res) {
        new Thread(() -> {
            String responseData = null;
            try (Socket socket = new Socket(targetip, 9999);
                 PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

                socket.setSoTimeout(10000);
                out.println("VOICE_CMD_" + res);
                String ans = in.readLine();
                responseData = ans;
            } catch (Exception e) {
                responseData = "LỖI: " + e.getMessage();
            }

            String finalData = responseData;
            runOnUiThread(() -> {
                if (finalData != null && finalData.startsWith("CLIP_DATA_")) {
                    String res2 = finalData.substring(10);
                    ClipboardManager clipMgr = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                    ClipData clip = ClipData.newPlainText("NeroClip", res2);
                    clipMgr.setPrimaryClip(clip);
                    Toast.makeText(this, "Đã đồng bộ Clipboard!", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "🎤 Server: " + finalData, Toast.LENGTH_SHORT).show();
                }
            });
        }).start();
    }

    private void send_socket_clipboard_command(String command) {
        new Thread(() -> {
            String responseData = null;
            try (Socket socket = new Socket(targetip, 9999);
                 PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                 BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

                socket.setSoTimeout(10000);
                out.println(command);
                String ans = in.readLine();
                responseData = ans;
            } catch (Exception e) {
                responseData = "LỖI: " + e.getMessage();
            }

            String finalData = responseData;
            runOnUiThread(() -> {
                dismiss_loading();
                if (finalData != null && finalData.startsWith("CLIP_DATA_")) {
                    String res = finalData.substring(10);
                    ClipboardManager clipMgr = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
                    ClipData clip = ClipData.newPlainText("NeroClip", res);
                    clipMgr.setPrimaryClip(clip);
                    Toast.makeText(this, "Đã đồng bộ Clipboard!", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "❌ " + finalData, Toast.LENGTH_SHORT).show();
                }
            });
        }).start();
    }

    private void start_live_stream() {
        new Thread(() -> {
            try {
                // Lấy IP WiFi của điện thoại
                android.net.wifi.WifiManager wifiManager = (android.net.wifi.WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                int ipAddress = wifiManager.getConnectionInfo().getIpAddress();
                String phoneIp = String.format(Locale.US, "%d.%d.%d.%d",
                        (ipAddress & 0xff), (ipAddress >> 8 & 0xff),
                        (ipAddress >> 16 & 0xff), (ipAddress >> 24 & 0xff));

                // Gửi lệnh bắt đầu stream tới server
                try (Socket socket = new Socket(targetip, 9999);
                     PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
                     BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()))) {

                    socket.setSoTimeout(10000);
                    out.println("LIVE_STREAM_" + phoneIp);
                    String ans = in.readLine();
                }

                runOnUiThread(() -> {
                    dismiss_loading();
                    // Mở LiveStreamActivity
                    Intent intent = new Intent(ControlActivity.this, LiveStreamActivity.class);
                    startActivity(intent);
                });
            } catch (Exception e) {
                runOnUiThread(() -> {
                    dismiss_loading();
                    Toast.makeText(this, "❌ Stream lỗi: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                });
            }
        }).start();
    }

    private Typeface getFontType(String fontName) {
        switch (fontName) {
            case "SERIF": return Typeface.SERIF;
            case "MONOSPACE": return Typeface.MONOSPACE;
            case "CURSIVE": return Typeface.create("cursive", Typeface.NORMAL);
            default: return Typeface.SANS_SERIF;
        }
    }

    private void applyFontToView(View view, Typeface typeface) {
        if (view instanceof TextView) {
            ((TextView) view).setTypeface(typeface);
        } else if (view instanceof ViewGroup) {
            ViewGroup vg = (ViewGroup) view;
            for (int i = 0; i < vg.getChildCount(); i++) {
                applyFontToView(vg.getChildAt(i), typeface);
            }
        }
    }
}