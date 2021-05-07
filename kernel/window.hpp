#pragma once

#include <vector>
#include <optional>
#include "graphics.hpp"
#include "frame_buffer.hpp"

/* 画面上の(Layer上の)矩形領域 */
class Window {
  public:
    // Window と関連付けられた PixelWriter
    class WindowWriter : public PixelWriter {
      public:
        WindowWriter(Window& window) : window_{window} {}

        virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
          window_.Write(pos, c);
        }

        virtual int Width() const override { return window_.Width(); }
        virtual int Height() const override { return window_.Height(); }

      private:
        Window& window_;
    };

    // 指定した大きさのウィンドウを生成
    Window(int width, int height, PixelFormat shadow_format);

    ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    // 与えられた FrameBuffer に、このウィンドウの表示領域を描画する
    void DrawTo(FrameBuffer& dst, Vector2D<int> position);

    // 透過色を設定
    void SetTransparentColor(std::optional<PixelColor> c);

    // 関連付けられている WindowWriter を取得
    WindowWriter* Writer();


    // 指定位置のピクセルの色を返す
    const PixelColor& At(Vector2D<int> pos) const;

    // 指定した位置に指定した色を描画
    void Write(Vector2D<int> pos, PixelColor c);
    
    // dst_pos に src の領域内の画像を移動させる
    void Move(Vector2D<int> dst_pos, const Rectangle<int>& src);
    
    // ウィンドウの幅・高さを取得
    int Width() const;
    int Height() const;

  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    FrameBuffer shadow_buffer_{};
};

void DrawWindow(PixelWriter& writer, const char* title);

