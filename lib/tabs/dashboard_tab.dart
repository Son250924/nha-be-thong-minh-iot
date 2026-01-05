import 'package:flutter/material.dart';

class DashboardTab extends StatelessWidget {
  final int gas, temp, hum, soil, accelVal;
  final int lightValue;
  final bool isAutoLamp;

  final bool isPump, isAuto, waveAlert, isLamp, isFan;
  final Function(bool) onToggleAuto, onTogglePump;
  final Function(bool) onToggleAutoLamp;

  final VoidCallback onToggleLamp, onToggleFan;
  final String cityName;
  final double weatherTemp;
  final String weatherDesc;
  final String weatherIconCode;
  final VoidCallback onShowWeatherDetail;

  const DashboardTab({
    super.key,
    required this.gas, required this.temp, required this.hum, required this.soil, required this.accelVal,
    required this.lightValue, required this.isAutoLamp,
    required this.isPump, required this.isAuto, required this.waveAlert, required this.isLamp, required this.isFan,
    required this.onToggleAuto, required this.onToggleAutoLamp, required this.onTogglePump, required this.onToggleLamp, required this.onToggleFan,
    required this.cityName, required this.weatherTemp, required this.weatherDesc, required this.weatherIconCode, required this.onShowWeatherDetail
  });

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          GestureDetector(
            onTap: onShowWeatherDetail,
            child: Container(
              padding: const EdgeInsets.all(16), margin: const EdgeInsets.only(bottom: 20),
              decoration: BoxDecoration(gradient: LinearGradient(colors: [Colors.blue.shade400, Colors.blue.shade700]), borderRadius: BorderRadius.circular(20), boxShadow: [BoxShadow(color: Colors.blue.withOpacity(0.3), blurRadius: 10, offset: const Offset(0, 5))]),
              child: Row(children: [Image.network("https://openweathermap.org/img/wn/$weatherIconCode@2x.png", width: 60, height: 60, errorBuilder: (c,e,s) => const Icon(Icons.cloud, color: Colors.white, size: 50)), const SizedBox(width: 10), Column(crossAxisAlignment: CrossAxisAlignment.start, children: [Row(children: [const Icon(Icons.location_on, color: Colors.white70, size: 14), const SizedBox(width: 4), Text(cityName.toUpperCase(), style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 16))]), Text(weatherDesc, style: const TextStyle(color: Colors.white70, fontSize: 13))]), const Spacer(), Text("${weatherTemp.toStringAsFixed(1)}°C", style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 32)), const SizedBox(width: 10), const Icon(Icons.arrow_forward_ios, color: Colors.white30, size: 16)]),
            ),
          ),
          GridView.count(
            shrinkWrap: true, physics: const NeverScrollableScrollPhysics(), crossAxisCount: 2, crossAxisSpacing: 12, mainAxisSpacing: 12, childAspectRatio: 1.5,
            children: [
              _buildCard("Khí Gas", "$gas", "ppm", Icons.cloud_circle, gas > 1800 ? Colors.red : Colors.green, gas > 1800),
              _buildCard("Rung lắc", "$accelVal", "m/s²", Icons.waves, waveAlert ? Colors.red : Colors.teal, waveAlert),
              _buildCard("Nhiệt độ", temp == -99 ? "--" : "$temp", temp == -99 ? "" : "°C", Icons.thermostat, temp == -99 ? Colors.grey : (temp > 40 ? Colors.red : Colors.orange), false),
              _buildCard("Ánh sáng", "$lightValue", "Lux", Icons.light_mode, Colors.amber, false),
              _buildCard("Độ ẩm Đất", "$soil", "%", Icons.grass, Colors.brown, false),
              _buildCard("Độ ẩm KK", hum == -99 ? "--" : "$hum", "%", Icons.water, Colors.blueAccent, false),
            ],
          ),
          const SizedBox(height: 20),
          const Text("TRUNG TÂM ĐIỀU KHIỂN", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.grey)),
          const SizedBox(height: 10),
          Card(
            elevation: 2, shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
            child: Column(children: [
              SwitchListTile(
                  title: const Text("Tự Động Bơm", style: TextStyle(fontWeight: FontWeight.bold)),
                  subtitle: const Text("Theo độ ẩm đất"),
                  secondary: const Icon(Icons.water_drop, color: Colors.blue),
                  value: isAuto,
                  onChanged: onToggleAuto
              ),
              const Divider(height: 1),
              SwitchListTile(
                  title: const Text("Tự Động bật Đèn", style: TextStyle(fontWeight: FontWeight.bold)),
                  subtitle: const Text("Bật khi trời tối"),
                  secondary: const Icon(Icons.lightbulb, color: Colors.orange),
                  value: isAutoLamp,
                  onChanged: onToggleAutoLamp
              ),
              const Divider(height: 1),
              ListTile(leading: Icon(Icons.water_drop, color: isPump ? Colors.blue : Colors.grey), title: const Text("Máy Bơm Nước", style: TextStyle(fontWeight: FontWeight.bold)), subtitle: Text(isAuto ? "Đang chạy theo Auto" : "Nhấn switch để Bật/Tắt"), trailing: Switch(value: isPump, onChanged: (isAuto) ? null : onTogglePump))
            ]),
          ),
          const SizedBox(height: 20),
          const Text("PHÍM TẮT NHANH", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.grey)),
          const SizedBox(height: 10),
          Row(children: [Expanded(child: _quickBtn("Đèn Chính", isLamp, Colors.orange, onToggleLamp, Icons.lightbulb)), const SizedBox(width: 15), Expanded(child: _quickBtn("Quạt Gió", isFan, Colors.blue, onToggleFan, Icons.wind_power))]),
        ],
      ),
    );
  }
  Widget _quickBtn(String label, bool isOn, Color color, VoidCallback onTap, IconData icon) { return GestureDetector(onTap: onTap, child: AnimatedContainer(duration: const Duration(milliseconds: 300), height: 80, decoration: BoxDecoration(color: isOn ? color : Colors.white, borderRadius: BorderRadius.circular(15), border: Border.all(color: color.withOpacity(0.5), width: 2), boxShadow: [if(isOn) BoxShadow(color: color.withOpacity(0.4), blurRadius: 10, offset: const Offset(0, 4))]), child: Column(mainAxisAlignment: MainAxisAlignment.center, children: [Icon(icon, color: isOn ? Colors.white : color, size: 30), const SizedBox(height: 5), Text(label, style: TextStyle(color: isOn ? Colors.white : Colors.black87, fontWeight: FontWeight.bold))]))); }
  Widget _buildCard(String title, String val, String unit, IconData icon, Color color, bool alert) { return Container(padding: const EdgeInsets.all(12), decoration: BoxDecoration(color: Colors.white, borderRadius: BorderRadius.circular(15), border: alert ? Border.all(color: Colors.red, width: 2) : null, boxShadow: [const BoxShadow(color: Colors.black12, blurRadius: 5)]), child: Column(crossAxisAlignment: CrossAxisAlignment.start, mainAxisAlignment: MainAxisAlignment.center, children: [Row(children: [Icon(icon, color: color, size: 24), const Spacer(), if(alert) const Icon(Icons.warning, color: Colors.red, size: 18)]), const Spacer(), Text(val, style: TextStyle(fontSize: 28, fontWeight: FontWeight.bold, color: color)), Text(title, style: const TextStyle(color: Colors.grey, fontSize: 12))])); }
}