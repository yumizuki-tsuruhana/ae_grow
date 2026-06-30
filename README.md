# YT Grow — After Effects Grow/Shrink Effect Plugin

高速な距離変換アルゴリズムを使用した、軽量かつパワフルなモルフォロジカル Grow/Shrink エフェクト。

## 特徴

- **超高速**: Felzenszwalb & Huttenlocher の距離変換アルゴリズム採用。半径に関係なく O(n) で処理完了
- **3つのモード**: Grow（膨張）/ Shrink（収縮）/ Edge Only（エッジ抽出）
- **3つのシェイプ**: Circle（円形）/ Square（四角）/ Diamond（ひし形）
- **チャンネル選択**: Alpha / Luminance / RGB Max で処理対象を選択
- **ソフトネス制御**: エッジの柔らかさを自由に調整
- **8bit / 16bit 対応**: Deep Color 完全サポート
- **SmartFX 対応**: AE の Smart Render パイプラインに対応し、必要最小限の領域のみ処理

## パラメータ

| パラメータ | 範囲 | 説明 |
|-----------|------|------|
| **Radius** | 0–500 px | Grow/Shrink する量 |
| **Softness** | 0–100% | エッジのぼかし量 |
| **Mode** | Grow / Shrink / Edge Only | 処理モード |
| **Shape** | Circle / Square / Diamond | カーネル形状 |
| **Channel** | Alpha / Luminance / RGB Max | 処理対象チャンネル |
| **Threshold** | 0–1.0 | エッジ検出の閾値 |
| **Invert** | On/Off | 結果を反転 |
| **Blend Original** | 0–100% | 元の画像とブレンド |

## ビルド方法

### 前提条件

- [Adobe After Effects SDK](https://developer.adobe.com/after-effects/) (無料ダウンロード)
- CMake 3.20 以上
- Windows: Visual Studio 2019+ / macOS: Xcode 12+

### ビルド手順

```bash
# 1. AE SDK のパスを指定してビルド
mkdir build && cd build
cmake .. -DAESDK_ROOT=/path/to/AfterEffectsSDK
cmake --build . --config Release

# Windows の場合
cmake .. -G "Visual Studio 17 2022" -A x64 -DAESDK_ROOT=C:/AfterEffectsSDK
cmake --build . --config Release
```

### インストール

ビルドされた `Grow.aex`（Windows）または `Grow.plugin`（macOS）を以下にコピー：

- **Windows**: `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\`
- **macOS**: `/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/`

After Effects を再起動すると、**Effect > YT Effects > YT Grow** に表示されます。

## アルゴリズム

距離変換ベースのアプローチを採用しています：

1. 入力画像からバイナリマスクを生成（閾値＋チャンネル選択）
2. 選択された形状に応じた距離変換を実行
   - Circle: ユークリッド距離変換（分離可能2パス）
   - Square: チェビシェフ距離変換
   - Diamond: マンハッタン距離変換
3. 距離値を Radius と Softness に基づいて 0–1 のファクターに変換
4. ファクターを出力に適用

従来のカーネルベースの膨張処理（O(n × r²)）と違い、距離変換は O(n) で完了するため、
Radius = 1 でも Radius = 500 でも処理時間はほぼ同じです。

## ライセンス

MIT License
