package com.example.remoteshutdown;

import android.graphics.Color;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import java.util.List;

public class DeviceAdapter extends RecyclerView.Adapter<DeviceAdapter.DeviceViewHolder> {

    private List<Device> listdevices;
    private ondeviceclicklistener listener;

    // Chỉ dùng 1 interface duy nhất để mở Bottom Sheet
    public interface ondeviceclicklistener {
        void ondeviceclick(Device device);
    }

    public DeviceAdapter(List<Device> listdevices, ondeviceclicklistener listener) {
        this.listdevices = listdevices;
        this.listener = listener;
    }

    @NonNull
    @Override
    public DeviceViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_device, parent, false);
        DeviceViewHolder ans = new DeviceViewHolder(view);
        return ans;
    }

    @Override
    public void onBindViewHolder(@NonNull DeviceViewHolder holder, int position) {
        Device device = listdevices.get(position);
        if (device == null) return;

        holder.tvdevicename.setText(device.getname());
        holder.tvdeviceip.setText(device.getip());

        String status = device.getstatus();
        holder.tvstatus.setText(status);

        if ("ONLINE".equals(status)) {
            holder.tvstatus.setTextColor(Color.parseColor("#4CAF50"));
        } else {
            holder.tvstatus.setTextColor(Color.parseColor("#F44336"));
        }

        // Chạm vào thẻ -> Đẩy device về MainActivity để nó mở Bottom Sheet lên
        holder.itemView.setOnClickListener(v -> listener.ondeviceclick(device));
    }

    @Override
    public int getItemCount() {
        int ans = 0;
        if (listdevices != null) {
            ans = listdevices.size();
        }
        return ans;
    }

    public static class DeviceViewHolder extends RecyclerView.ViewHolder {
        TextView tvdevicename, tvdeviceip, tvstatus;

        public DeviceViewHolder(@NonNull View itemview) {
            super(itemview);
            // Cắt sạch đống Button đi vì chúng nó không còn ở đây nữa
            tvdevicename = itemview.findViewById(R.id.tv_device_name);
            tvdeviceip = itemview.findViewById(R.id.tv_device_ip);
            tvstatus = itemview.findViewById(R.id.tv_status);
        }
    }
}