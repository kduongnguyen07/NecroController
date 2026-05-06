package com.example.remoteshutdown;

public class Device {
    private String name;
    private String ip;
    private String status;
    private long lastseen;
    private String mac; // THÊM LỖI DÒNG NÀY ĐỂ NHỚ MAC

    public Device() {
    }

    public Device(String name, String ip, String status, long lastseen, String mac) {
        this.name = name;
        this.ip = ip;
        this.status = status;
        this.lastseen = lastseen;
        this.mac = mac;
    }

    public String getname() { return name; }
    public void setname(String name) { this.name = name; }

    public String getip() { return ip; }
    public void setip(String ip) { this.ip = ip; }

    public String getstatus() { return status; }
    public void setstatus(String status) { this.status = status; }

    public long getlastseen() { return lastseen; }
    public void setlastseen(long lastseen) { this.lastseen = lastseen; }

    public String getmac() { return mac; }
    public void setmac(String mac) { this.mac = mac; }
}