# EQ-1

JUCEフレームワークによる、FabFilter Pro-Q 3のようなビジュアル操作のパラメトリックEQプラグイン。
旧「DemoEQ」(Projucer + JUCE 7.0.3 + Visual Studio)をCMake構成に移植し、その後
固定5バンド・スキューモーフィックUIから、可変バンド数・フラットダークUIへ全面刷新した。

## 指示
- 返答は日本語で行うこと

## 構成
- JUCE本体は `../JUCE`(plugins配下の共有)を参照
- 画像アセットは使用していない(フラットUIはすべてベクター描画)。`assets/`ディレクトリと
  `juce_add_binary_data`は刷新時に削除済み
- `Source/DSP/` … フィルタ係数計算(FilterBand)とFFTスペクトラムアナライザー
- `Source/GUI/` … EQGraphComponent(メインのインタラクティブグラフ)、BandControlStrip
  (選択中バンドの数値編集)、TopBarComponent、FlatLookAndFeel
- バンドは最大16個まで(`EQ1AudioProcessor::maxNumBands`)。APVTSは全バンド分のパラメータを
  あらかじめ確保し、`band{n}_enabled`で表示/非表示を切り替える方式
- グラフ上の操作: クリックでバンド追加/選択、ドラッグでFreq・Gain変更、ホイールでQ変更、
  右クリックでフィルタ種別・スロープ変更、ダブルクリックまたはDeleteキーでバンド削除

## ビルド
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## 実行
```
./build/EQ1_artefacts/Release/Standalone/EQ-1
```
