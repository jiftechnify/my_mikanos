#pragma once

#include <vector>
#include <optional>
#include "graphics.hpp"

/* 画面上の(Layer上の)矩形領域 */
class Window {
  public:
    // Window と関連付けられた PixelWriter
    class WindowWriter : public PixelWriter {
      public:
        WindowWriter(Window& window) : window_{window} {}

        virtual void Write(int x, int y, const PixelColor& c) override {
          window_.At(x, y) = c;
        }

        virtual int Width() const override { return window_.Width(); }
        virtual int Height() const override { return window_.Height(); }

      private:
        Window& window_;
    };

    // 指定した大きさのウィンドウを生成
    Window(int width, int height);

    ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    // 与えられた PixelWriter に、このウィンドウの表示領域を描画する
    void DrawTo(PixelWriter& writer, Vector2D<int> position);

    // 透過色を設定
    void SetTransparentColor(std::optional<PixelColor> c);

    // 関連付けられている WindowWriter を取得
    WindowWriter* Writer();


    // 指定位置のピクセルの色を返す
    PixelColor& At(int x, int y);
    const PixelColor& At(int x, int y) const;

    // ウィンドウの幅・高さを取得
    int Width() const;
    int Height() const;

  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};
};

