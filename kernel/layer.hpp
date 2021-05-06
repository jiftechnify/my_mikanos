#pragma once

#include <memory>
#include <map>
#include <vector>

#include "graphics.hpp"
#include "window.hpp"

/* 画面上の描画レイヤ */
class Layer {
  public:
    // 指定したIDを持つレイヤを生成
    Layer(unsigned int id = 0);
    
    // レイヤのIDを取得
    unsigned int ID() const;

    
    // ウィンドウをレイヤに設定する
    Layer& SetWindow(const std::shared_ptr<Window>& window);

    // 設定されたウィンドウを返す
    std::shared_ptr<Window> GetWindow() const;

    
    // レイヤの原点を指定された座標に移動する
    Layer& Move(Vector2D<int> pos);

    // レイヤの原点を現在の位置から指定された方向に移動する
    Layer& MoveRelative(Vector2D<int> pos_diff);


    // 指定された FrmaeBuffer に、このレイヤに設定されているウィンドウを描画
    void DrawTo(FrameBuffer& screen) const;

  private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
};

/* 重なり合う複数のレイヤを管理するクラス */
class LayerManager {
  public:
    // 描画先の FrameBuffer を設定
    void SetWriter(FrameBuffer* screen);

    // 新しいレイヤを生成し、参照を返す
    Layer& NewLayer();


    // 表示状態にあるレイヤを描画
    void Draw() const;


    // 指定したレイヤを指定座標に移動
    void Move(unsigned int id, Vector2D<int> new_position);

    // 指定したレイヤを指定方向に移動
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);


    // レイヤの奥行き方向の位置を設定
    // 負の値を設定すると、レイヤは非表示になる
    void UpDown(unsigned int id, int new_height);

    // レイヤを非表示にする
    void Hide(unsigned int id);

  private:
    FrameBuffer* screen_{nullptr};
    std::vector<std::unique_ptr<Layer>> layers_{};  // レイヤを管理する配列。レイヤオブジェクトの「所有者」になる
    std::vector<Layer*> layer_stack_{};             // レイヤの重なり(奥行き方向の位置関係)を表現する
    unsigned int latest_id_{0};

    Layer* FindLayer(unsigned int id);
};

extern LayerManager* layer_manager;

