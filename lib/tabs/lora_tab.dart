import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

class LoraTab extends StatefulWidget {
  final List<String> messages;
  final Function(String) onSOS;
  final Function(String) onSendMessage;

  final double lat, long;
  final int satCount;
  final bool hasFix;
  final String loraStatus;

  const LoraTab({
    super.key,
    required this.messages,
    required this.onSOS,
    required this.onSendMessage,
    this.lat = 0.0,
    this.long = 0.0,
    this.satCount = 0,
    this.hasFix = false,
    this.loraStatus = "Đang kết nối..."
  });

  @override
  State<LoraTab> createState() => _LoraTabState();
}

class _LoraTabState extends State<LoraTab> with TickerProviderStateMixin {
  late AnimationController _progressController;
  late AnimationController _pulseController;
  late Animation<double> _pulseAnimation;

  bool isHolding = false;
  final TextEditingController _msgController = TextEditingController();
  Timer? _hapticTimer;

  @override
  void initState() {
    super.initState();
    _progressController = AnimationController(vsync: this, duration: const Duration(seconds: 3));
    _pulseController = AnimationController(vsync: this, duration: const Duration(seconds: 1));
    _pulseAnimation = Tween<double>(begin: 1.0, end: 1.1).animate(
        CurvedAnimation(parent: _pulseController, curve: Curves.easeInOut)
    );
    _pulseController.repeat(reverse: true);

    _progressController.addStatusListener((status) {
      if (status == AnimationStatus.completed) {
        _triggerSOS();
      }
    });
  }

  void _triggerSOS() {
    widget.onSOS("SOS");
    HapticFeedback.heavyImpact();
    _resetSOS();

    showDialog(
        context: context,
        builder: (ctx) => AlertDialog(
            title: const Text("ĐÃ GỬI SOS KHẨN CẤP!", style: TextStyle(color: Colors.red, fontWeight: FontWeight.bold)),
            content: const Column(
              mainAxisSize: MainAxisSize.min,
              children: [
                Icon(Icons.warning_amber_rounded, color: Colors.red, size: 60),
                SizedBox(height: 10),
                Text("Tín hiệu cầu cứu đã được gửi qua LoRa kèm TỌA ĐỘ GPS hiện tại.", textAlign: TextAlign.center),
              ],
            ),
            actions: [TextButton(onPressed: () => Navigator.pop(ctx), child: const Text("OK"))]
        )
    );
  }

  void _resetSOS() {
    _progressController.reset();
    _hapticTimer?.cancel();
    setState(() => isHolding = false);
  }

  void _onTapDown() {
    setState(() => isHolding = true);
    _progressController.forward();
    _hapticTimer = Timer.periodic(const Duration(milliseconds: 100), (t) {
      HapticFeedback.selectionClick();
    });
  }

  void _onTapUp() {
    if (!_progressController.isCompleted) {
      _resetSOS();
    }
  }

  @override
  void dispose() {
    _progressController.dispose();
    _pulseController.dispose();
    _msgController.dispose();
    _hapticTimer?.cancel();
    super.dispose();
  }

  void _handleSend() {
    if (_msgController.text.trim().isNotEmpty) {
      widget.onSendMessage(_msgController.text.trim());
      _msgController.clear();
      FocusScope.of(context).unfocus();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Expanded(
          child: ListView(
            padding: const EdgeInsets.all(20.0),
            children: [
              Card(
                  color: Colors.white, elevation: 2,
                  shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(15)),
                  child: Padding(
                      padding: const EdgeInsets.all(16.0),
                      child: Column(children: [
                        Row(children: [const Icon(Icons.router, color: Colors.purple), const SizedBox(width: 10), const Text("LoRa Network:", style: TextStyle(fontWeight: FontWeight.bold)), const Spacer(), Text(widget.loraStatus, style: const TextStyle(color: Colors.green, fontWeight: FontWeight.bold))]),
                        const Divider(height: 20),
                        Row(crossAxisAlignment: CrossAxisAlignment.start, children: [
                          Icon(Icons.satellite_alt, color: widget.hasFix ? Colors.blue : Colors.orange),
                          const SizedBox(width: 10),
                          Expanded(child: Column(crossAxisAlignment: CrossAxisAlignment.start, children: [
                            Row(mainAxisAlignment: MainAxisAlignment.spaceBetween, children: [Text(widget.hasFix ? "GPS Đã Định Vị" : "Đang tìm vệ tinh...", style: TextStyle(fontWeight: FontWeight.bold, color: widget.hasFix ? Colors.black87 : Colors.orange)), Container(padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 2), decoration: BoxDecoration(color: Colors.grey.shade200, borderRadius: BorderRadius.circular(5)), child: Text("${widget.satCount} Sats", style: const TextStyle(fontSize: 10, fontWeight: FontWeight.bold)))]),
                            const SizedBox(height: 5),
                            if (widget.hasFix) Text("${widget.lat.toStringAsFixed(6)}, ${widget.long.toStringAsFixed(6)}", style: const TextStyle(fontSize: 16, color: Colors.blue, fontWeight: FontWeight.bold, letterSpacing: 1)) else const Text("Đang chờ tín hiệu từ mạch...", style: TextStyle(fontSize: 12, color: Colors.grey))
                          ]))
                        ])
                      ])
                  )
              ),
              const SizedBox(height: 40),
              Center(
                child: GestureDetector(
                  onTapDown: (_) => _onTapDown(),
                  onTapUp: (_) => _onTapUp(),
                  onTapCancel: () => _onTapUp(),
                  child: Stack(alignment: Alignment.center, children: [
                    if (isHolding)
                      ScaleTransition(
                        scale: _pulseAnimation,
                        child: Container(
                          width: 240, height: 240,
                          decoration: BoxDecoration(
                            shape: BoxShape.circle,
                            color: Colors.red.withOpacity(0.2),
                          ),
                        ),
                      ),


                    // SizedBox(
                    //     width: 220, height: 220,
                    //     child: CircularProgressIndicator(
                    //       value: isHolding ? null : 0,
                    //
                    //       // value: isHolding ? _progressController.value : 0,
                    //       valueColor: const AlwaysStoppedAnimation<Color>(Colors.red),
                    //       strokeWidth: 8,
                    //       backgroundColor: Colors.grey.shade200,
                    //     )
                    // ),

                    SizedBox(
                      width: 220, height: 220,
                      child: AnimatedBuilder(
                        animation: _progressController,
                        builder: (context, child) {
                          return CircularProgressIndicator(
                            value: isHolding ? _progressController.value : 0,
                            strokeWidth: 15,
                            backgroundColor: Colors.transparent,
                            valueColor: const AlwaysStoppedAnimation<Color>(Colors.redAccent),
                            strokeCap: StrokeCap.round,
                          );
                        },
                      ),
                    ),
                    Container(
                        height: 180, width: 180,
                        decoration: BoxDecoration(
                            shape: BoxShape.circle,
                            gradient: LinearGradient(
                                begin: Alignment.topLeft, end: Alignment.bottomRight,
                                colors: [Colors.red.shade400, Colors.red.shade900]
                            ),
                            boxShadow: [
                              BoxShadow(color: Colors.red.withOpacity(0.5), blurRadius: 20, spreadRadius: 5, offset: const Offset(0, 10))
                            ]
                        ),
                        child: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: const [
                              Icon(Icons.sos_outlined, size: 50, color: Colors.white),
                              SizedBox(height: 5),
                              Text("GIỮ 3 GIÂY", style: TextStyle(color: Colors.white70, fontSize: 12, fontWeight: FontWeight.w500))
                            ]
                        )
                    )
                  ]),
                ),
              ),

              const SizedBox(height: 40),
              const Divider(),
              const Align(alignment: Alignment.centerLeft, child: Text("Nhật ký Tin Nhắn LoRa", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.grey))),

              ListView.builder(
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  itemCount: widget.messages.length,
                  itemBuilder: (ctx, i) {
                    String msg = widget.messages[i];
                    bool isMe = msg.startsWith("TX:");
                    return Container(
                      margin: const EdgeInsets.symmetric(vertical: 4),
                      alignment: isMe ? Alignment.centerRight : Alignment.centerLeft,
                      child: Container(
                        padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                        decoration: BoxDecoration(
                          color: isMe ? Colors.blue.shade100 : Colors.white,
                          border: isMe ? null : Border.all(color: Colors.grey.shade300),
                          borderRadius: BorderRadius.only(
                            topLeft: const Radius.circular(12),
                            topRight: const Radius.circular(12),
                            bottomLeft: isMe ? const Radius.circular(12) : const Radius.circular(0),
                            bottomRight: isMe ? const Radius.circular(0) : const Radius.circular(12),
                          ),
                        ),
                        child: Text(msg.substring(4), style: const TextStyle(fontSize: 14)),
                      ),
                    );
                  }
              )
            ],
          ),
        ),
        Container(
          padding: const EdgeInsets.all(10.0),
          decoration: BoxDecoration(
              color: Colors.white,
              boxShadow: [BoxShadow(offset: const Offset(0, -2), blurRadius: 10, color: Colors.black.withOpacity(0.05))]
          ),
          child: Row(
            children: [
              Expanded(
                child: TextField(
                  controller: _msgController,
                  decoration: InputDecoration(
                    hintText: "Nhập tin nhắn...",
                    contentPadding: const EdgeInsets.symmetric(horizontal: 20, vertical: 12),
                    border: OutlineInputBorder(borderRadius: BorderRadius.circular(30), borderSide: BorderSide.none),
                    filled: true,
                    fillColor: Colors.grey.shade100,
                  ),
                ),
              ),
              const SizedBox(width: 10),
              CircleAvatar(
                radius: 24,
                backgroundColor: Colors.blue,
                child: IconButton(
                  icon: const Icon(Icons.send, color: Colors.white, size: 20),
                  onPressed: _handleSend,
                ),
              )
            ],
          ),
        ),
      ],
    );
  }
}