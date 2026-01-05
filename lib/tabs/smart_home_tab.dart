import 'package:flutter/material.dart';

class SmartHomeTab extends StatelessWidget {
  final bool isLamp, isFan, isPump;
  final Function(String, bool) onToggle;
  const SmartHomeTab({super.key, required this.isLamp, required this.isFan, required this.isPump, required this.onToggle});
  @override
  Widget build(BuildContext context) {
    return ListView(padding: const EdgeInsets.all(16), children: [
      _sectionTitle("Hệ thống Chiếu sáng"), _deviceTile("Đèn Chiếu Sáng", isLamp, Icons.lightbulb, Colors.orange, (v) => onToggle("LAMP", v)),
      const SizedBox(height: 20), _sectionTitle("Hệ thống Làm mát & Bơm"), _deviceTile("Quạt", isFan, Icons.wind_power, Colors.blue, (v) => onToggle("FAN", v)),
      Card(elevation: 2, margin: const EdgeInsets.only(bottom: 10), child: ListTile(leading: Container(padding: const EdgeInsets.all(8), decoration: BoxDecoration(color: isPump ? Colors.blue.withOpacity(0.2) : Colors.grey.shade100, shape: BoxShape.circle), child: Icon(Icons.water_drop, color: isPump ? Colors.blue : Colors.grey)), title: const Text("Máy Bơm Nước", style: TextStyle(fontWeight: FontWeight.w600)), subtitle: const Text("Điều khiển tại Dashboard (chế độ Auto)"), trailing: Switch(value: isPump, onChanged: (v) => onToggle("PUMP", v))))
    ]);
  }
  Widget _sectionTitle(String title) => Padding(padding: const EdgeInsets.only(bottom: 10), child: Text(title, style: const TextStyle(fontWeight: FontWeight.bold, color: Colors.grey, fontSize: 13)));
  Widget _deviceTile(String name, bool isOn, IconData icon, Color color, Function(bool)? onChanged) { return Card(elevation: 2, margin: const EdgeInsets.only(bottom: 10), child: SwitchListTile(title: Text(name, style: const TextStyle(fontWeight: FontWeight.w600)), secondary: Container(padding: const EdgeInsets.all(8), decoration: BoxDecoration(color: isOn ? color.withOpacity(0.2) : Colors.grey.shade100, shape: BoxShape.circle), child: Icon(icon, color: isOn ? color : Colors.grey)), value: isOn, onChanged: onChanged, activeColor: color)); }
}