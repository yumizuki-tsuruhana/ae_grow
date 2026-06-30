# YT Grow — After Effects Grow/Shrink エフェクト

軽量かつパワフルなモルフォロジカル Grow/Shrink エフェクト。

## 2つの使い方

### 🎯 スクリプト版（おすすめ・コンパイル不要！）

`YT_Grow.jsx` をドロップするだけ。開発環境は一切不要。

### ⚙️ C++ プラグイン版（上級者向け）

距離変換アルゴリズムによるネイティブプラグイン。Adobe AE SDK が必要。

---

## スクリプト版のインストール

### Step 1: `YT_Grow.jsx` をコピー

- **Windows**: `C:\Program Files\Adobe\Adobe After Effects <version>\Support Files\Scripts\ScriptUI Panels\`
- **macOS**: `/Applications/Adobe After Effects <version>/Scripts/ScriptUI Panels/`

### Step 2: AE を再起動

### Step 3: パネルを開く

**Window > YT Grow** で表示されます。

### 使い方

1. コンポジションでレイヤーを選択
2. YT Grow パネルで Radius や Mode を設定
3. 「Apply to Selected Layer」をクリック

### パラメータ

| パラメータ | 説明 |
|-----------|------|
| **Radius** | Grow/Shrink の量（0–200 px） |
| **Softness** | エッジのぼかし量（0–100） |
| **Mode** | Grow（膨張）/ Shrink（収縮）/ Grow + Shrink |
| **Shape** | Circle / Square / Diamond |
| **Channel** | Alpha のみ / All（RGB + Alpha） |
| **Quality** | Fast(1x) / Medium(2x) / High(3x) — 大きい Radius 時の精度 |

---

## C++ プラグイン版（上級者向け）

### 必要なもの

- [Adobe After Effects SDK](https://developer.adobe.com/after-effects/) (無料・要Adobe ID)
- CMake 3.20 以上
- Windows: Visual Studio 2019+ / macOS: Xcode 12+

### ビルド手順

```bash
mkdir build && cd build
cmake .. -DAESDK_ROOT=/path/to/AfterEffectsSDK
cmake --build . --config Release
```

### インストール

ビルドされた `Grow.aex`（Win）/ `Grow.plugin`（Mac）をコピー：

- **Windows**: `C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore\`
- **macOS**: `/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/`

AE 再起動 → **Effect > YT Effects > YT Grow**

### アルゴリズム

Felzenszwalb & Huttenlocher の距離変換を使い、半径に関係なく O(n) で処理完了。
従来のカーネル方式（O(n × r²)）と比べて大きい Radius でも速度が落ちません。

## ライセンス

MIT License
