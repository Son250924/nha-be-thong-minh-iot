import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:shared_preferences/shared_preferences.dart';

import '../globals.dart';
import '../tabs/dashboard_tab.dart';
import '../tabs/smart_home_tab.dart';
import '../tabs/lora_tab.dart';
import '../tabs/settings_tab.dart';
import 'intro_page.dart';

class MainAppScreen extends StatefulWidget {
  final String? savedDeviceId;
  const MainAppScreen({super.key, required this.savedDeviceId});
  @override
  State<MainAppScreen> createState() => _MainAppScreenState();
}

class _MainAppScreenState extends State<MainAppScreen> {
  int _currentIndex = 0;
  BluetoothDevice? _device;
  bool isConnected = false;
  int gas=0, temp=0, hum=0, soil=0, accelVal=0;
  bool isPump = false, isAuto = true, waveAlert = false, isLamp = false, isFan = false;
  double gpsLat = 0.0, gpsLon = 0.0;
  int satCount = 0;
  bool hasGPSFix = false;
  int lightValue = 0;
  bool isAutoLamp = true;
  String openWeatherApiKey = "afe165931f131e85312bf93b1ac55150";
  String weatherDesc = "Đang tải...";
  String weatherIcon = "10d";
  double weatherTemp = 0.0;
  int weatherHum = 0;
  double windSpeed = 0.0;
  double limitGas = 1800;
  double limitTemp = 45;
  double limitSoilLower = 60;
  double limitWave = 14;
  double limitLight = 2000;

  double limitHumLow = 40, limitHumHigh = 90;

  String city = "Ha Long";
  bool enableNotifications = true;
  bool enableSoilNotif = true;
  bool enablePumpNotif = true;

  List<String> loraMessages = [];
  String _lastReceivedLoRaMsg = "";
  DateTime? _lastLoRaTime;
  DateTime? _lastGasNotif, _lastTempNotif;
  DateTime? _lastWaveNotif;
  DateTime? _pumpStartTime;

  final String serviceUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  final String charTxUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
  final String charRxUUID = "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e";

  BluetoothCharacteristic? rxCharacteristic;
  StreamSubscription? connectionStateSubscription;

  @override
  void initState() {
    super.initState();
    _loadSettings();
    if (widget.savedDeviceId != null) {
      _attemptAutoReconnect(widget.savedDeviceId!);
    } else {
      _startDemoData();
    }
    Future.delayed(const Duration(seconds: 1), _fetchWeatherData);
  }

  Future<void> _loadSettings() async {
    final prefs = await SharedPreferences.getInstance();
    setState(() {
      city = prefs.getString('saved_city') ?? "Ha Long";
      limitGas = prefs.getDouble('limit_gas') ?? 1800;
      limitTemp = prefs.getDouble('limit_temp') ?? 45;
      limitSoilLower = prefs.getDouble('limit_soil') ?? 60;
      limitWave = prefs.getDouble('limit_wave') ?? 14;
      limitLight = prefs.getDouble('limit_light') ?? 2000;
    });
  }

  Future<void> _fetchWeatherData() async {
    try {
      final client = HttpClient();
      final urlOWM = Uri.parse("https://api.openweathermap.org/data/2.5/weather?q=$city&units=metric&lang=vi&appid=$openWeatherApiKey");
      final request = await client.getUrl(urlOWM);
      final response = await request.close();
      if (response.statusCode == 200) {
        final jsonString = await response.transform(utf8.decoder).join();
        final data = jsonDecode(jsonString);
        setState(() {
          weatherTemp = (data['main']['temp'] as num).toDouble();
          weatherHum = data['main']['humidity'];
          windSpeed = (data['wind']['speed'] as num).toDouble();
          weatherDesc = data['weather'][0]['description'];
          weatherIcon = data['weather'][0]['icon'];
          if (weatherDesc.isNotEmpty) weatherDesc = weatherDesc[0].toUpperCase() + weatherDesc.substring(1);
        });
      }
    } catch (e) { print("Lỗi Weather: $e"); }
  }

  void _showWeatherDetailDialog() {
    showDialog(context: context, builder: (ctx) => AlertDialog(
      backgroundColor: Colors.blue.shade800, contentPadding: EdgeInsets.zero,
      content: Container(
        height: 350, decoration: BoxDecoration(gradient: LinearGradient(begin: Alignment.topLeft, end: Alignment.bottomRight, colors: [Colors.blue.shade400, Colors.blue.shade900]), borderRadius: BorderRadius.circular(15)),
        child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [
          Text(city.toUpperCase(), style: const TextStyle(color: Colors.white, fontSize: 24, fontWeight: FontWeight.bold, letterSpacing: 1.5)),
          const SizedBox(height: 10),
          Image.network("https://openweathermap.org/img/wn/$weatherIcon@4x.png", width: 100, height: 100, errorBuilder: (_,__,___)=>const Icon(Icons.cloud, size: 80, color: Colors.white)),
          Text("$weatherTemp°C", style: const TextStyle(color: Colors.white, fontSize: 50, fontWeight: FontWeight.bold)),
          Text(weatherDesc, style: const TextStyle(color: Colors.white70, fontSize: 18)),
          const SizedBox(height: 20),
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _weatherDetailItem(Icons.water_drop, "$weatherHum%", "Độ ẩm"),
              _weatherDetailItem(Icons.air, "$windSpeed m/s", "Gió"),
              _weatherDetailItem(Icons.visibility, "10km", "Tầm nhìn"), // Demo value
            ],
          )
        ],
        ),
      ),
    ));
  }

  Widget _weatherDetailItem(IconData icon, String val, String label) => Column(children: [Icon(icon, color: Colors.white, size: 24), const SizedBox(height: 5), Text(val, style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold)), Text(label, style: const TextStyle(color: Colors.white60, fontSize: 10))]);

  void _startDemoData() {
    Timer.periodic(const Duration(seconds: 2), (t) {
      if (mounted && widget.savedDeviceId == null) {
        setState(() {
          isConnected = true;
          gas = 400 + Random().nextInt(100);
          temp = 28 + Random().nextInt(4);
          hum = 70 + Random().nextInt(10);
          soil = 60 + Random().nextInt(5);
          accelVal = 9;
          lightValue = 1500 + Random().nextInt(500);
        });
      }
    });
  }

  Future<void> _attemptAutoReconnect(String deviceId) async {
    setState(() => isConnected = false);
    try {
      _device = BluetoothDevice.fromId(deviceId);
      connectionStateSubscription = _device!.connectionState.listen((state) {
        if (mounted) setState(() => isConnected = (state == BluetoothConnectionState.connected));
        if (state == BluetoothConnectionState.connected) { _discoverServices(); }
        else if (state == BluetoothConnectionState.disconnected) { Future.delayed(const Duration(seconds: 5), () { if (mounted && !isConnected) _device?.connect(autoConnect: true).catchError((e){}); }); }
      });
      await _device!.connect(autoConnect: true);
    } catch (e) { print("Lỗi kết nối lại: $e"); }
  }

  Future<void> _discoverServices() async {
    if (_device == null) return;
    try {
      List<BluetoothService> services = await _device!.discoverServices();
      for (var s in services) {
        if (s.uuid.toString().toLowerCase() == serviceUUID) {
          for (var c in s.characteristics) {
            if (c.uuid.toString().toLowerCase() == charTxUUID) { await c.setNotifyValue(true); c.lastValueStream.listen(_onDataReceived); }
            if (c.uuid.toString().toLowerCase() == charRxUUID) { rxCharacteristic = c; Future.delayed(const Duration(milliseconds: 500), _syncTime); }
          }
        }
      }
    } catch (e) { print("Lỗi dịch vụ: $e"); }
  }

  void _syncTime() { sendCommand("TIME|${DateTime.now().millisecondsSinceEpoch ~/ 1000}"); }

  void _onDataReceived(List<int> value) {
    if (value.isEmpty) return;
    String packet = utf8.decode(value);
    try {
      List<String> parts = packet.split('|');
      if (packet.startsWith("LORA|")) {
        String msg = packet.substring(5);
        _handleNewLoRaMsg(msg);
      }
      else if (parts.length >= 17) {
        if (mounted) {
          bool prevPump = isPump;
          setState(() {
            gas = int.tryParse(parts[0]) ?? 0;
            temp = int.tryParse(parts[1]) ?? -99;
            hum = int.tryParse(parts[2]) ?? -99;
            soil = int.tryParse(parts[3]) ?? 0;
            isPump = parts[4] == "1";
            isAuto = parts[5] == "1";
            waveAlert = parts[6] == "1";
            accelVal = int.tryParse(parts[7]) ?? 0;
            isLamp = parts[8] == "1";
            isFan  = parts[9] == "1";

            double lat = double.tryParse(parts[10]) ?? 0.0;
            double lon = double.tryParse(parts[11]) ?? 0.0;
            satCount = int.tryParse(parts[12]) ?? 0;
            if (lat != 0.0 && lon != 0.0) { gpsLat = lat; gpsLon = lon; hasGPSFix = true; } else { hasGPSFix = false; }

            lightValue = int.tryParse(parts[15]) ?? 0;
            isAutoLamp = parts[16] == "1";
          });

          String rawLoRaMsg = parts[13];
          if (rawLoRaMsg.isNotEmpty && rawLoRaMsg != "Chua co tin") { _handleNewLoRaMsg(rawLoRaMsg); }
          _checkAlertsAndNotify(prevPump);
        }
      }
    } catch (e) { print("Lỗi Parse: $e"); }
  }

  void _handleNewLoRaMsg(String msg) {
    DateTime now = DateTime.now();


    if (msg.trim().isEmpty || msg == "Chua co tin") return;

//spam
    bool isSameMessage = (msg == _lastReceivedLoRaMsg);
    bool isTimeOut = (_lastLoRaTime == null || now.difference(_lastLoRaTime!).inSeconds > 5);

    if (isSameMessage && !isTimeOut) {
      return;
    }
    _lastReceivedLoRaMsg = msg;
    _lastLoRaTime = now;

    String displayMsg = "RX: $msg";
    bool isUrgent = false;

    if (msg.startsWith("MSG_CENTER|")) {
      displayMsg = "RX: Trạm: " + msg.substring(16);
    } else if (msg.startsWith("CONFIRM_SOS|")) {
      displayMsg = "RX: Trạm: " + msg.substring(12);
      isUrgent = true;
    } else if (msg.contains("SOS")) {
      isUrgent = true;
    }

    setState(() => loraMessages.insert(0, displayMsg));

    if (enableNotifications) {
      if (msg.contains("SOS") && !msg.contains("CONFIRM_SOS")) {
        _showNotification("CẢNH BÁO KHẨN CẤP", "Nhận được tín hiệu SOS: $msg");
        HapticFeedback.heavyImpact();
      }
      else if (msg.startsWith("MSG_CENTER|")) {
        _showNotification("TIN NHẮN TỪ TRUNG TÂM", msg.substring(16));
      }
      else if (msg.startsWith("CONFIRM_SOS|")) {
        _showNotification("ĐÃ XÁC NHẬN CỨU HỘ", msg.substring(12));
        HapticFeedback.mediumImpact();
      }
      else {
        _showNotification("TIN NHẮN LORA", msg);
      }
    }
  }

  void _checkAlertsAndNotify(bool prevPump) {
    if (!enableNotifications) return;
    DateTime now = DateTime.now();

    if (accelVal > limitWave) {
      HapticFeedback.heavyImpact();
      if (_lastWaveNotif == null || now.difference(_lastWaveNotif!).inSeconds >= 15) {
        _showNotification("CẢNH BÁO RUNG LẮC", "Sóng lớn ($accelVal m/s²). Kiểm tra ngay!");
        _lastWaveNotif = now;
      }
    }
    if (gas > limitGas) {
      if (_lastGasNotif == null || now.difference(_lastGasNotif!).inMinutes >= 1) { _showNotification("NGUY HIỂM: KHÍ GAS", "Nồng độ: $gas ppm."); _lastGasNotif = now; }
    }
    if (temp > limitTemp && temp != -99) {
      if (_lastTempNotif == null || now.difference(_lastTempNotif!).inMinutes >= 5) { _showNotification("CẢNH BÁO NHIỆT ĐỘ", "Nhiệt độ cao: $temp°C."); _lastTempNotif = now; }
    }
    if (enablePumpNotif) {
      if (!prevPump && isPump) { _pumpStartTime = DateTime.now(); _showNotification("MÁY BƠM", "Đang tưới cây..."); }
      else if (prevPump && !isPump) {
        if (_pumpStartTime != null) { Duration d = DateTime.now().difference(_pumpStartTime!); _showNotification("MÁY BƠM", "Đã xong (${d.inSeconds}s)."); _pumpStartTime = null; }
        else { _showNotification("MÁY BƠM", "Đã tắt."); }
      }
    }
    if (enableSoilNotif && soil < limitSoilLower && !isPump) { if (now.minute % 30 == 0 && now.second < 2) { _showNotification("CÂY CẦN NƯỚC", "Đất khô ($soil%)."); } }
  }

  Future<void> _showNotification(String title, String body) async {
    const AndroidNotificationDetails android = AndroidNotificationDetails('smart_home_channel', 'Cảnh báo Nhà Bè', importance: Importance.max, priority: Priority.high, color: Colors.red);
    const NotificationDetails details = NotificationDetails(android: android);
    await flutterLocalNotificationsPlugin.show(Random().nextInt(1000), title, body, details);
  }

  void sendCommand(String cmd) async {
    if (rxCharacteristic != null) {
      try { await rxCharacteristic!.write(utf8.encode(cmd)); HapticFeedback.lightImpact(); } catch (e) { print("Lỗi gửi lệnh: $e"); }
    } else {
      if (cmd.startsWith("SOS")) { setState(() { loraMessages.insert(0, "TX: SOS sent at ${DateTime.now().toString().substring(11,19)}"); }); }
    }
  }

  Future<void> _logout() async {
    final prefs = await SharedPreferences.getInstance(); await prefs.remove('saved_device_id'); await _device?.disconnect();
    if (mounted) Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => const IntroPage()));
  }

  @override
  void dispose() { connectionStateSubscription?.cancel(); super.dispose(); }

  @override
  Widget build(BuildContext context) {
    final commonAppBar = AppBar(
      backgroundColor: Colors.white, elevation: 1,
      title: Row(children: [
        Container(padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 5), decoration: BoxDecoration(color: isConnected ? Colors.green.shade50 : Colors.red.shade50, borderRadius: BorderRadius.circular(15), border: Border.all(color: isConnected ? Colors.green : Colors.red)), child: Row(children: [Icon(Icons.circle, size: 10, color: isConnected ? Colors.green : Colors.red), const SizedBox(width: 6), Text(isConnected ? "Đã kết nối" : "Mất kết nối", style: TextStyle(fontSize: 12, fontWeight: FontWeight.bold, color: isConnected ? Colors.green : Colors.red))])),
        const Spacer(), const Text("NHÀ BÈ THÔNG MINH", style: TextStyle(color: Colors.blue, fontWeight: FontWeight.bold, fontSize: 20)), const SizedBox(width: 8),
      ]),
    );

    final List<Widget> pages = [
      DashboardTab(
        gas: gas, temp: temp, hum: hum, soil: soil, accelVal: accelVal,
        isPump: isPump, isAuto: isAuto, waveAlert: waveAlert, isLamp: isLamp, isFan: isFan,
        lightValue: lightValue, isAutoLamp: isAutoLamp,
        cityName: city, weatherTemp: weatherTemp, weatherDesc: weatherDesc, weatherIconCode: weatherIcon, onShowWeatherDetail: _showWeatherDetailDialog,
        onToggleAuto: (val) => sendCommand(val ? "AUTO_ON" : "AUTO_OFF"),
        onToggleAutoLamp: (val) => sendCommand(val ? "AUTO_LAMP_ON" : "AUTO_LAMP_OFF"),
        onTogglePump: (val) => sendCommand(val ? "PUMP_ON" : "PUMP_OFF"),
        onToggleLamp: () => sendCommand(isLamp ? "LAMP_OFF" : "LAMP_ON"),
        onToggleFan: () => sendCommand(isFan ? "FAN_OFF" : "FAN_ON"),
      ),
      SmartHomeTab(
          isLamp: isLamp, isFan: isFan, isPump: isPump,
          onToggle: (type, val) {
            if (type == "LAMP") sendCommand(val ? "LAMP_ON" : "LAMP_OFF");
            if (type == "FAN") sendCommand(val ? "FAN_ON" : "FAN_OFF");
            if (type == "PUMP") sendCommand(val ? "PUMP_ON" : "PUMP_OFF");
          }
      ),
      LoraTab(
        messages: loraMessages, lat: gpsLat, long: gpsLon, satCount: satCount, hasFix: hasGPSFix, loraStatus: isConnected ? "BLE: Đã Kết Nối" : "Mất kết nối",
        onSOS: (String msg) {
          sendCommand(msg);
          setState(() { loraMessages.insert(0, "TX: SOS REQUEST"); });
        },
        onSendMessage: (String msg) {
          sendCommand("MSG|$msg");
          setState(() { loraMessages.insert(0, "TX: Tôi: $msg"); });
        },
      ),
      SettingsTab(
        limitGas: limitGas, limitTemp: limitTemp, limitSoil: limitSoilLower, limitWave: limitWave, limitLight: limitLight,
        limitHumLow: limitHumLow, limitHumHigh: limitHumHigh,
        city: city, openWeatherApiKey: openWeatherApiKey,
        enableNotif: enableNotifications, enableSoilNotif: enableSoilNotif, enablePumpNotif: enablePumpNotif,
        onUpdateLimits: (g, t, s, w, l, hl, hh) async {
          setState(() { limitGas=g; limitTemp=t; limitSoilLower=s; limitWave=w; limitLight=l; limitHumLow=hl; limitHumHigh=hh; });
          final prefs = await SharedPreferences.getInstance();
          await prefs.setDouble('limit_gas', g); await prefs.setDouble('limit_temp', t); await prefs.setDouble('limit_soil', s); await prefs.setDouble('limit_wave', w); await prefs.setDouble('limit_light', l);
          String cmd = "SET_LIMITS|${g.toInt()}|${t.toInt()}|${s.toInt()}|${w.toStringAsFixed(1)}|${l.toInt()}";
          sendCommand(cmd);
        },
        onUpdateWeatherConfig: (newCity, newKey) async { setState(() { city = newCity; openWeatherApiKey = newKey; }); final prefs = await SharedPreferences.getInstance(); await prefs.setString('saved_city', newCity); _fetchWeatherData(); },
        onToggleNotif: (val) => setState(() => enableNotifications = val), onToggleSoilNotif: (val) => setState(() => enableSoilNotif = val), onTogglePumpNotif: (val) => setState(() => enablePumpNotif = val), onLogout: _logout,
      ),
    ];

    return Scaffold(
      appBar: commonAppBar, body: pages[_currentIndex],
      bottomNavigationBar: NavigationBar(
        selectedIndex: _currentIndex, onDestinationSelected: (int index) => setState(() { _currentIndex = index; HapticFeedback.selectionClick(); }),
        destinations: const [
          NavigationDestination(icon: Icon(Icons.dashboard_outlined), selectedIcon: Icon(Icons.dashboard), label: 'Tổng quan'),
          NavigationDestination(icon: Icon(Icons.home_mini_outlined), selectedIcon: Icon(Icons.home_mini), label: 'Thiết bị'),
          NavigationDestination(icon: Icon(Icons.emergency_share_outlined), selectedIcon: Icon(Icons.emergency_share), label: 'Cứu hộ'),
          NavigationDestination(icon: Icon(Icons.settings_outlined), selectedIcon: Icon(Icons.settings), label: 'Cài đặt'),
        ],
      ),
    );
  }
}