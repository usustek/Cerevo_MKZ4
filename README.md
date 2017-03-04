# Cerevo_MKZ4
Cerevo MKZ4 kai


## 何するの？

Cerevo MKZ4は前後左右On/Offの操作しか、標準のソフトウェアではできません。
なめらかな操作ができたら良いよね！ということで改造を始めました。

## 必要なもの

- アナログスティックのついたゲームパッド
  - いまのところ Logicool GamePad F310のみ対応
- 上記ゲームパッドを認識してWiFiに接続可能なPC
- Cerevo MKZ4を組み込んだワイルドミニ四駆

## 使用しているライブラリ

- ESPAsyncTCP
  - https://github.com/me-no-dev/ESPAsyncTCP
- ESPAsyncWebServer
  - https://github.com/me-no-dev/ESPAsyncWebServer
- DRV8830MotorDriver
  - https://gist.github.com/gundamsan/98efc45afd877dff7254
- ArduinoJson
  - https://github.com/bblanchon/ArduinoJson

## ソフトウェアの概要

- MKZ4(ESP8266)側
 - WebServerとなってPC/スマホの操作用UIをホスティング
- PC/スマホ
 - 操作用のformデータを送信

 WebSocket化したESP8266側のソフトウェアを間違って消してしまった(´；ω；`)ので、
 現在ビルドしてもまともに動きません。
