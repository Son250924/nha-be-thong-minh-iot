import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'main_app_screen.dart';
import 'dart:io';
class IntroPage extends StatefulWidget {
  const IntroPage({super.key});
  @override
  State<IntroPage> createState() => _IntroPageState();
}

class _IntroPageState extends State<IntroPage> {
  String _status = "";
  bool _isScanning = false;
  final String targetDeviceName = "NHA_BE_SMART";
  final String serviceUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  final String charTxUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

  Future<void> _startConnection() async {
    if (Platform.isAndroid || Platform.isIOS) {
      await [
        Permission.bluetoothScan,
        Permission.bluetoothConnect,
        Permission.location,
        Permission.notification
      ].request();
    }
    if (!mounted) return;
    setState(() { _isScanning = true; _status = "Đang tìm thiết bị..."; });

    await FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));

    StreamSubscription? scanSub;
    scanSub = FlutterBluePlus.scanResults.listen((results) {
      for (ScanResult r in results) {
        if (r.device.platformName == targetDeviceName) {
          FlutterBluePlus.stopScan();
          scanSub?.cancel();
          _setupSecureConnection(r.device);
          break;
        }
      }
    });

    Future.delayed(const Duration(seconds: 6), () {
      if (mounted && _isScanning) {
        setState(() {
          _isScanning = false;
          _status = "Không tìm thấy '$targetDeviceName'.\nHãy kiểm tra nguồn điện mạch.";
        });
      }
    });
  }

  Future<void> _setupSecureConnection(BluetoothDevice device) async {
    setState(() => _status = "Đang xác thực...");
    try {
      await device.connect(autoConnect: false);

      if (!mounted) return;

      showDialog(
        context: context,
        barrierDismissible: false,
        builder: (ctx) => AlertDialog(
          title: const Text("Yêu cầu xác thực"),
          content: const Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Icon(Icons.touch_app, size: 50, color: Colors.blue),
              SizedBox(height: 10),
              Text("BẤM NÚT TRÊN MẠCH 2 LẦN\nđể chấp nhận kết nối.", textAlign: TextAlign.center),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () {
                device.disconnect();
                Navigator.pop(ctx);
                setState(() { _isScanning = false; _status = "Đã hủy kết nối."; });
              },
              child: const Text("Hủy"),
            )
          ],
        ),
      );

      List<BluetoothService> services = await device.discoverServices();
      BluetoothCharacteristic? txChar;
      for (var s in services) {
        if (s.uuid.toString().toLowerCase() == serviceUUID) {
          for (var c in s.characteristics) {
            if (c.uuid.toString().toLowerCase() == charTxUUID) txChar = c;
          }
        }
      }

      if (txChar != null) {
        await txChar.setNotifyValue(true);
        StreamSubscription? sub;
        sub = txChar.lastValueStream.listen((value) async {
          if (value.isNotEmpty) {
            sub?.cancel();
            if (mounted && Navigator.canPop(context)) Navigator.pop(context);

            final prefs = await SharedPreferences.getInstance();
            await prefs.setString('saved_device_id', device.remoteId.str);

            _navigateToMain(device.remoteId.str);
          }
        });
      }
    } catch (e) {
      if (mounted && Navigator.canPop(context)) Navigator.pop(context);
      setState(() { _isScanning = false; _status = "Lỗi kết nối: $e"; });
      device.disconnect();
    }
  }

  void _navigateToMain(String deviceId) {
    if (!mounted) return;
    Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => MainAppScreen(savedDeviceId: deviceId)));
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.white,
      body: SafeArea(
        child: Center(
          child: SingleChildScrollView(
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.center,
              children: [
                const Icon(Icons.home_work_rounded, size: 100, color: Colors.blue),
                const SizedBox(height: 20),
                const Text("NHÀ BÈ THÔNG MINH", style: TextStyle(fontSize: 32, fontWeight: FontWeight.bold, color: Colors.blue)),
                const SizedBox(height: 10),
                const Text("Hệ thống giám sát & Cứu hộ", style: TextStyle(fontSize: 16, color: Colors.grey)),
                const Text("V1", style: TextStyle(fontSize: 12, color: Colors.blueGrey)),
                const SizedBox(height: 50),

                if (_status.isNotEmpty)
                  Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Text(_status, textAlign: TextAlign.center, style: TextStyle(color: _isScanning ? Colors.blue : Colors.red, fontWeight: FontWeight.bold)),
                  ),

                const SizedBox(height: 10),

                SizedBox(
                  width: 250, height: 50,
                  child: ElevatedButton.icon(
                    icon: _isScanning ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(color: Colors.white, strokeWidth: 2)) : const Icon(Icons.bluetooth_searching),
                    onPressed: _isScanning ? null : _startConnection,
                    label: Text(_isScanning ? " ĐANG QUÉT..." : " KẾT NỐI THIẾT BỊ"),
                    style: ElevatedButton.styleFrom(backgroundColor: Colors.blue, foregroundColor: Colors.white),
                  ),
                ),

                const SizedBox(height: 20),
                if (!_isScanning)
                  TextButton(
                    onPressed: () => Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => const MainAppScreen(savedDeviceId: null))),
                    child: const Text("Chạy chế độ Demo (Không cần mạch)", style: TextStyle(color: Colors.grey)),
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}