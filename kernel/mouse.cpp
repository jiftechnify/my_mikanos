#include "mouse.hpp"

#include <limits>
#include <memory>
#include "graphics.hpp"
#include "layer.hpp"
#include "usb/classdriver/mouse.hpp"
#include "task.hpp"

namespace {
  const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
    "@              ",
    "@@             ",
    "@.@            ",
    "@..@           ",
    "@...@          ",
    "@....@         ",
    "@.....@        ",
    "@......@       ",
    "@.......@      ",
    "@........@     ",
    "@.........@    ",
    "@..........@   ",
    "@...........@  ",
    "@............@ ",
    "@......@@@@@@@@",
    "@......@       ",
    "@....@@.@      ",
    "@...@ @.@      ",
    "@..@   @.@     ",
    "@.@    @.@     ",
    "@@      @.@    ",
    "@       @.@    ",
    "         @.@   ",
    "         @@@   ",
  };

  std::pair<Layer*, uint64_t> FindActiveLayerTask() {
    const auto act_layer_id = active_layer->GetActive();
    if (!act_layer_id) {
      return { nullptr, 0 };
    }
    const auto layer = layer_manager->FindLayer(act_layer_id);
  
    const auto task_it = layer_task_map->find(act_layer_id);
    if (task_it == layer_task_map->end()) {
      return { layer, 0 };
    }

    return { layer, task_it->second };
  } 

  // アクティブレイヤに紐つくタスクにマウス移動メッセージを送る
  void SendMouseMessage(Vector2D<int> newpos, Vector2D<int> posdiff, uint8_t buttons, uint8_t prev_buttons) {
    const auto [ layer, task_id ] = FindActiveLayerTask();
    if (!layer || !task_id) {
      return;
    }
  
    const auto relpos = newpos - layer->GetPosition();
    if (posdiff.x != 0 || posdiff.y != 0) {
      Message msg{Message::kMouseMove};
      msg.arg.mouse_move.x = relpos.x;
      msg.arg.mouse_move.y = relpos.y;
      msg.arg.mouse_move.dx = posdiff.x;
      msg.arg.mouse_move.dy = posdiff.y;
      msg.arg.mouse_move.buttons = buttons;
      task_manager->SendMessage(task_id, msg);
    }

    if (prev_buttons != buttons) {
      const auto diff = prev_buttons ^ buttons;
      for (int i = 0; i < 8; ++i) {
        if ((diff >> i) & 1) {
          Message msg{Message::kMouseButton};
          msg.arg.mouse_button.x = relpos.x;
          msg.arg.mouse_button.y = relpos.y;
          msg.arg.mouse_button.press = (buttons >> i) & 1;
          msg.arg.mouse_button.button = i;
          task_manager->SendMessage(task_id, msg);
        }
      }
    }
  }
  void SendCloseMessage() {
    const auto [ layer, task_id ] = FindActiveLayerTask();
    if (!layer || !task_id) {
      return;
    }
  
    Message msg{Message::kWindowClose};
    msg.arg.window_close.layer_id = layer->ID();
    task_manager->SendMessage(task_id, msg);
  }
}


void DrawMouseCursor(PixelWriter* pixel_writer, Vector2D<int> position) {
  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, {0, 0, 0});
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, {255, 255, 255});
      } else {
        pixel_writer->Write(position + Vector2D<int>{dx, dy}, kMouseTransparentColor);
      }
    }
  }
}

Mouse::Mouse(unsigned int layer_id) : layer_id_{layer_id} {
}

void Mouse::SetPosition(Vector2D<int> position) {
  position_ = position;
  layer_manager->Move(layer_id_, position_);
}

void Mouse::OnInterrupt(uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
  const auto oldpos = position_;
  auto newpos = position_ + Vector2D<int>{displacement_x, displacement_y};
  newpos = ElementMin(newpos, ScreenSize() + Vector2D<int>{-1, -1});
  position_ = ElementMax(newpos, Vector2D<int>{0, 0});

  const auto posdiff = position_ - oldpos;

  layer_manager->Move(layer_id_, position_);

  // クリック・ドラッグを検出し、
  // ・ウィンドウの閉じるボタンが押されたらウィンドウを閉じる
  // ・ドラッグ開始時にマウスポインタの下にあったレイヤーをドラッグに伴って移動させる
  unsigned int close_layer_id = 0;

  const bool prev_left_pressed = (prev_buttons_ & 0x01);
  const bool left_pressed = (buttons & 0x01);
  if (!prev_left_pressed && left_pressed) {
    // ドラッグ開始
    auto layer = layer_manager->FindLayerByPosition(position_, layer_id_);
    if (layer && layer->IsDraggable()) {
      const auto pos_layer = position_ - layer->GetPosition();
      switch (layer->GetWindow()->GetWindowRegion(pos_layer)) {
      case WindowRegion::kTitleBar:
        drag_layer_id_ = layer->ID();
        break;
      case WindowRegion::kCloseButton:
        close_layer_id = layer->ID();
        break;
      default:
        break;
      }

      active_layer->Activate(layer->ID());
    } else {
      // ドラッグ不可レイヤをクリックした場合はすべてのウィンドウを非アクティブに
      active_layer->Activate(0);
    }
  } else if (prev_left_pressed && left_pressed) {
    // ドラッグ中
    if (drag_layer_id_ > 0) {
      layer_manager->MoveRelative(drag_layer_id_, posdiff);
    }
  } else if (prev_left_pressed && !left_pressed) {
    // ドラッグ終了
    drag_layer_id_ = 0;
  }

  if (drag_layer_id_ == 0) {
    if (close_layer_id == 0) {
      SendMouseMessage(newpos, posdiff, buttons, prev_buttons_);
    } else {
      SendCloseMessage();
    }
  }

  prev_buttons_ = buttons;
}

void InitializeMouse() {
  
  auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, screen_config.pixel_format);
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0, 0});

  auto mouse_layer_id = layer_manager->NewLayer()
    .SetWindow(mouse_window)
    .ID();

  auto mouse = std::make_shared<Mouse>(mouse_layer_id);
  mouse->SetPosition({200, 200});
  layer_manager->UpDown(mouse->LayerID(), std::numeric_limits<int>::max());
  
  usb::HIDMouseDriver::default_observer =
    [mouse](uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
      mouse->OnInterrupt(buttons, displacement_x, displacement_y);
    };

  active_layer->SetMouseLayer(mouse_layer_id);
}
