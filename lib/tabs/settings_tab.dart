import 'package:flutter/material.dart';

class SettingsTab extends StatefulWidget {
  final double limitGas, limitTemp, limitSoil, limitWave, limitHumLow, limitHumHigh;
  final double limitLight;
  final String city, openWeatherApiKey;
  final bool enableNotif, enableSoilNotif, enablePumpNotif;

  final Function(double, double, double, double, double, double, double) onUpdateLimits;
  final Function(String, String) onUpdateWeatherConfig;
  final Function(bool) onToggleNotif, onToggleSoilNotif, onTogglePumpNotif;
  final VoidCallback onLogout;

  const SettingsTab({
    super.key,
    required this.limitGas, required this.limitTemp, required this.limitSoil, required this.limitWave, required this.limitLight,
    required this.limitHumLow, required this.limitHumHigh,
    required this.city, required this.openWeatherApiKey, required this.enableNotif, required this.enableSoilNotif, required this.enablePumpNotif,
    required this.onUpdateLimits, required this.onUpdateWeatherConfig, required this.onToggleNotif, required this.onToggleSoilNotif, required this.onTogglePumpNotif, required this.onLogout
  });

  @override
  State<SettingsTab> createState() => _SettingsTabState();
}

class _SettingsTabState extends State<SettingsTab> {

  late TextEditingController _cityCtrl;
  late TextEditingController _keyCtrl;

  @override
  void initState() {
    super.initState();
    _cityCtrl = TextEditingController(text: widget.city);
    _keyCtrl = TextEditingController(text: widget.openWeatherApiKey);
  }

  @override
  void dispose() {
    _cityCtrl.dispose();
    _keyCtrl.dispose();
    super.dispose();
  }

  @override
  void didUpdateWidget(SettingsTab oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (oldWidget.city != widget.city) {
    }
  }

  @override
  Widget build(BuildContext context) {
    return ListView(padding: const EdgeInsets.all(16), children: [
      Text("Phiên bản phần mềm: V1"),
      _header("CẤU HÌNH THỜI TIẾT"),
      Card(
          elevation: 2,
          child: Padding(
              padding: const EdgeInsets.all(12.0),
              child: Column(children: [
                TextField(
                    controller: _cityCtrl,
                    decoration: const InputDecoration(labelText: "Tên Thành Phố (VD: Ha Long)", prefixIcon: Icon(Icons.location_city))
                ),
                const SizedBox(height: 10),
                ElevatedButton.icon(
                    onPressed: () {
                      widget.onUpdateWeatherConfig(_cityCtrl.text, widget.openWeatherApiKey);
                      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text("Đã cập nhật!")));
                    },
                    icon: const Icon(Icons.save),
                    label: const Text("Lưu & Cập nhật")
                )
              ])
          )
      ),

      const SizedBox(height: 20),
      _header("QUẢN LÝ THÔNG BÁO"),
      Card(
          elevation: 2,
          child: Column(children: [
            SwitchListTile(
                title: const Text("Thông báo tổng"),
                subtitle: const Text("Bật/Tắt toàn bộ cảnh báo"),
                secondary: const Icon(Icons.notifications_active, color: Colors.red),
                value: widget.enableNotif,
                onChanged: widget.onToggleNotif
            ),

            if (widget.enableNotif) ...[
              const Divider(height: 1, indent: 50),
              SwitchListTile(
                  title: const Text("Hoạt động Máy Bơm"),
                  subtitle: const Text("Báo khi Bơm bật/tắt"),
                  secondary: const Icon(Icons.water_drop, color: Colors.blue, size: 20),
                  value: widget.enablePumpNotif,
                  onChanged: widget.onTogglePumpNotif,
                  dense: true
              ),
              const Divider(height: 1, indent: 50),
              SwitchListTile(
                  title: const Text("Độ ẩm Đất"),
                  subtitle: const Text("Nhắc nhở khi cây cần nước"),
                  secondary: const Icon(Icons.grass, color: Colors.brown, size: 20),
                  value: widget.enableSoilNotif,
                  onChanged: widget.onToggleSoilNotif,
                  dense: true
              ),
              const Divider(height: 1, indent: 50),
              const ListTile(
                leading: Icon(Icons.security, color: Colors.orange, size: 20),
                title: Text("Cảnh báo An toàn", style: TextStyle(color: Colors.grey)),
                subtitle: Text("Gas, Nhiệt độ, Rung lắc (Luôn bật)", style: TextStyle(fontSize: 12)),
                dense: true,
                trailing: Icon(Icons.check_circle, color: Colors.green, size: 18),
              ),
            ]
          ])
      ),

      const SizedBox(height: 20),
      _header("NGƯỠNG CẢNH BÁO & TỰ ĐỘNG"),
      Card(elevation: 2, child: Column(children: [
        _slider("Gas Max", widget.limitGas, 500, 4095, "ppm", (v) => widget.onUpdateLimits(v, widget.limitTemp, widget.limitSoil, widget.limitWave, widget.limitLight, widget.limitHumLow, widget.limitHumHigh)),
        const Divider(height: 1),
        _slider("Nhiệt độ Max", widget.limitTemp, 20, 60, "°C", (v) => widget.onUpdateLimits(widget.limitGas, v, widget.limitSoil, widget.limitWave, widget.limitLight, widget.limitHumLow, widget.limitHumHigh)),
        const Divider(height: 1),
        _slider("Độ ẩm Đất Min (Bật Bơm)", widget.limitSoil, 0, 100, "%", (v) => widget.onUpdateLimits(widget.limitGas, widget.limitTemp, v, widget.limitWave, widget.limitLight, widget.limitHumLow, widget.limitHumHigh)),
        const Divider(height: 1),
        _slider("Ngưỡng Rung Lắc", widget.limitWave, 5, 25, "m/s²", (v) => widget.onUpdateLimits(widget.limitGas, widget.limitTemp, widget.limitSoil, v, widget.limitLight, widget.limitHumLow, widget.limitHumHigh)),
        const Divider(height: 1),
        _slider("Ngưỡng Ánh Sáng (Bật Đèn)", widget.limitLight, 0, 4095, "Lux", (v) => widget.onUpdateLimits(widget.limitGas, widget.limitTemp, widget.limitSoil, widget.limitWave, v, widget.limitHumLow, widget.limitHumHigh)),
      ])),

      const SizedBox(height: 30),
      SizedBox(width: double.infinity, child: OutlinedButton.icon(icon: const Icon(Icons.logout), label: const Text("HỦY LIÊN KẾT THIẾT BỊ"), onPressed: widget.onLogout, style: OutlinedButton.styleFrom(foregroundColor: Colors.red, padding: const EdgeInsets.all(15))))
    ]);
  }

  Widget _header(String t) => Padding(padding: const EdgeInsets.only(bottom: 8, left: 5), child: Text(t, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.grey, fontSize: 13)));
  Widget _slider(String title, double val, double min, double max, String unit, Function(double) onChanged) { return Padding(padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8), child: Column(children: [Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [Text(title), Text("${val.toInt()} $unit", style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.blue))]), Slider(value: val, min: min, max: max, divisions: 100, onChanged: onChanged)])); }
}