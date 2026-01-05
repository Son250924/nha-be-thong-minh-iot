import 'package:flutter/material.dart';
import 'package:flutter_local_notifications/flutter_local_notifications.dart';
import 'package:shared_preferences/shared_preferences.dart';
import 'globals.dart';
import 'screens/intro_page.dart';
import 'screens/main_app_screen.dart';
import 'dart:io';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();


  if (Platform.isAndroid || Platform.isIOS) {
    const AndroidInitializationSettings initializationSettingsAndroid = AndroidInitializationSettings('@mipmap/ic_launcher');
    const InitializationSettings initializationSettings = InitializationSettings(android: initializationSettingsAndroid);
    await flutterLocalNotificationsPlugin.initialize(initializationSettings);
  }

  runApp(const MaterialApp(
    debugShowCheckedModeBanner: false,
    title: 'Nhà Bè Thông Minh',
    home: AppEntryPoint(),
  ));
}

class AppEntryPoint extends StatefulWidget {
  const AppEntryPoint({super.key});
  @override
  State<AppEntryPoint> createState() => _AppEntryPointState();
}

class _AppEntryPointState extends State<AppEntryPoint> {
  @override
  void initState() {
    super.initState();
    _checkSavedDevice();
  }

  Future<void> _checkSavedDevice() async {
    final prefs = await SharedPreferences.getInstance();
    final savedId = prefs.getString('saved_device_id');
    await Future.delayed(const Duration(seconds: 1));
    if (!mounted) return;

    if (savedId != null && savedId.isNotEmpty) {
      Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => MainAppScreen(savedDeviceId: savedId)));
    } else {
      Navigator.pushReplacement(context, MaterialPageRoute(builder: (context) => const IntroPage()));
    }
  }

  @override
  Widget build(BuildContext context) {
    return const Scaffold(
      backgroundColor: Colors.white,
      body: Center(child: CircularProgressIndicator()),
    );
  }
}