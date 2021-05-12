#pragma once

#include <vector>
#include <optional>
#include <string>
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

    virtual ~Window() = default;
    Window(const Window& rhs) = delete;
    Window& operator=(const Window& rhs) = delete;

    // 与えられた FrameBuffer に、このウィンドウの表示領域を描画する
    void DrawTo(FrameBuffer& dst, Vector2D<int> pos);
    void DrawTo(FrameBuffer& dst, Vector2D<int> pos, const Rectangle<int>& area);

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
    Vector2D<int> Size() const;

    // アクティブ状態切り替え(実装は各種サブクラスが行う)
    virtual void Activate() {}
    virtual void Deactivate() {}

  private:
    int width_, height_;
    std::vector<std::vector<PixelColor>> data_{};
    WindowWriter writer_{*this};
    std::optional<PixelColor> transparent_color_{std::nullopt};

    FrameBuffer shadow_buffer_{};
};

class ToplevelWindow : public Window {
  public:
    static constexpr Vector2D<int> kTopLeftMargin{4, 24};
    static constexpr Vector2D<int> kBottomRightMargin{4, 4};
    static constexpr int kMarginX = kTopLeftMargin.x + kBottomRightMargin.x;
    static constexpr int kMarginY = kTopLeftMargin.y + kBottomRightMargin.y;

    class InnerAreaWriter: public PixelWriter {
      public:
        InnerAreaWriter(ToplevelWindow& window) : window_{window} {}
        virtual void Write(Vector2D<int> pos, const PixelColor& c) override {
          window_.Write(pos + kTopLeftMargin, c);
        }
        virtual int Width() const override {
          return window_.Width() - kTopLeftMargin.x - kBottomRightMargin.x;
        }
        virtual int Height() const override {
          return window_.Height() - kTopLeftMargin.y - kBottomRightMargin.y;
        }

      private:
        ToplevelWindow& window_;
    };

    ToplevelWindow(int width, int height, PixelFormat shadow_format, const std::string& title);

    virtual void Activate() override;
    virtual void Deactivate() override;

    InnerAreaWriter* InnerWriter() { return &inner_writer_; }
    Vector2D<int> InnerSize() const;

  private:
    std::string title_;
    InnerAreaWriter inner_writer_{*this};
};

void DrawWindow(PixelWriter& writer, const char* title);
void DrawWindowTitle(PixelWriter& writer, const char *title, bool active);

void DrawTextbox(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);
void DrawTerminal(PixelWriter& writer, Vector2D<int> pos, Vector2D<int> size);

