#pragma once

#include <memory>
#include <map>
#include <vector>

#include "graphics.hpp"
#include "window.hpp"
#include "message.hpp"

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

    // レイヤーの座標
    Vector2D<int> GetPosition() const;
    
    // レイヤの原点を指定された座標に移動する
    Layer& Move(Vector2D<int> pos);

    // レイヤの原点を現在の位置から指定された方向に移動する
    Layer& MoveRelative(Vector2D<int> pos_diff);


    // 指定された FrmaeBuffer に、このレイヤに設定されているウィンドウを描画
    void DrawTo(FrameBuffer& screen) const;
    
    void DrawTo(FrameBuffer& screen, const Rectangle<int>& area) const;

  
    // ドラッグ可能フラグを設定
    Layer& SetDraggable(bool draggable); 

    // ドラッグ可能かどうか
    bool IsDraggable() const;

  private:
    unsigned int id_;
    Vector2D<int> pos_;
    std::shared_ptr<Window> window_;
    bool draggable_{false};
};

/* 重なり合う複数のレイヤを管理するクラス */
class LayerManager {
  public:
    // 描画先の FrameBuffer を設定
    void SetWriter(FrameBuffer* screen);

    // 新しいレイヤを生成し、参照を返す
    Layer& NewLayer();
    // 指定したレイヤを削除
    void RemoveLayer(unsigned int id);

    // 画面全体を再描画
    void DrawAll() const;
    // 指定領域を再描画
    void Draw(const Rectangle<int>& area) const;
    // 指定したレイヤとそれより前面にあるレイヤのみ再描画
    void Draw(unsigned int id) const;
    // 指定したレイヤより前面のレイヤの、指定範囲のみ再描画
    void Draw(unsigned int id, Rectangle<int> area) const;

    // 指定したレイヤを指定座標に移動
    void Move(unsigned int id, Vector2D<int> new_pos);

    // 指定したレイヤを指定方向に移動
    void MoveRelative(unsigned int id, Vector2D<int> pos_diff);


    // レイヤの奥行き方向の位置を設定
    // 負の値を設定すると、レイヤは非表示になる
    void UpDown(unsigned int id, int new_height);

    // レイヤを非表示にする
    void Hide(unsigned int id);

    // 指定位置にある最前面のレイヤを取得
    Layer* FindLayerByPosition(Vector2D<int> pos, unsigned int exclude_id) const;
    // 指定IDのレイヤを取得
    Layer* FindLayer(unsigned int id);
    
    // 指定IDのレイヤの高さ
    int GetHeight(unsigned int id);
  

  private:
    FrameBuffer* screen_{nullptr};
    mutable FrameBuffer back_buffer_{}; // 実際にフレームバッファに描画する前に仮描画するためのバッファ
    std::vector<std::unique_ptr<Layer>> layers_{};  // レイヤを管理する配列。レイヤオブジェクトの「所有者」になる
    std::vector<Layer*> layer_stack_{};             // レイヤの重なり(奥行き方向の位置関係)を表現する
    unsigned int latest_id_{0};

};

extern LayerManager* layer_manager;

void InitializeLayer();
void ProcessLayerMessage(const Message& msg);

constexpr Message MakeLayerMessage(
    uint64_t task_id, unsigned int layer_id,
    LayerOperation op, const Rectangle<int>& area) {
  Message msg{Message::kLayer, task_id};
  msg.arg.layer.layer_id = layer_id;
  msg.arg.layer.op = op;
  msg.arg.layer.x = area.pos.x;
  msg.arg.layer.y = area.pos.y;
  msg.arg.layer.w = area.size.x;
  msg.arg.layer.h = area.size.y;
  return msg;
}

/* 選択されたレイヤをアクティブにする責務を持つクラス */
class ActiveLayer {
  public:
    ActiveLayer(LayerManager& manager);
    void SetMouseLayer(unsigned int mouse_layer_id);
    void Activate(unsigned int layer_id);
    unsigned int GetActive() const { return active_layer_id_; }

  private:
    LayerManager& manager_;
    unsigned int active_layer_id_{0};
    unsigned int mouse_layer_id_{0};
};

extern ActiveLayer* active_layer;
extern std::map<unsigned int, uint64_t>* layer_task_map;

