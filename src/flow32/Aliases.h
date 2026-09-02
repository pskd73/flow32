#pragma once

/** Namespaced public aliases; legacy upstream symbols remain source-compatible. */
namespace flow32 {

using Runtime = ::Flow32;
using RuntimeConfig = ::FlowConfig;
using Flow32 = ::Flow32;
using FlowConfig = ::FlowConfig;
using AppBase = ::AppBase;
using AppHost = ::AppHost;
using AppInfo = ::AppInfo;
using Shell = ::Shell;
using NavMode = ::NavMode;
using Error = ::FlowError;
using FatalHandler = ::FlowFatalHandler;
using ::flowFatalHandler;
using ::setFlowFatalHandler;

template <typename State> using App = ::App<State>;
template <typename State> using StateStore = ::AppStore<State>;

namespace graphics {
using Panel = ::DisplayPanel;
using DisplayPanel = ::DisplayPanel;
using Display = ::Display;
using Transport = ::DisplayTransport;
using DisplayTransport = ::DisplayTransport;
using St77xxTransport = ::St77xxTransport;
using St77xxConfig = ::St77xxConfig;
using PanelChip = ::PanelChip;
using Canvas = ::Canvas;
// Retained here for compatibility; Page is implemented in the UI subsystem.
using Page = ::Page;
using Rect = ::Rect;
using FontPack = ::FontPack;
using FontFace = ::FontFace;
using AAFont = ::AAFont;
} // namespace graphics

namespace assets {
using Store = ::AssetStore;
using AssetStore = ::AssetStore;
using FilesystemStore = ::FsAssetStore;
using FsAssetStore = ::FsAssetStore;
using SdStore = ::Storage;
using Storage = ::Storage;
using SdConfig = ::StorageConfig;
using StorageConfig = ::StorageConfig;
using SdBus = ::SdBus;
using IconAtlas = ::IconAtlas;
using IconGlyph = ::IconGlyph;
using StreamedIcons = ::StreamedIconAtlas;
using StreamedIconAtlas = ::StreamedIconAtlas;
using EmojiAtlas = ::ColorEmojiAtlas;
using EmojiGlyph = ::ColorEmojiGlyph;
using StreamedEmoji = ::StreamedEmojiAtlas;
using StreamedEmojiAtlas = ::StreamedEmojiAtlas;
} // namespace assets

namespace input {
using Hub = ::InputHub;
using InputHub = ::InputHub;
using Source = ::InputSource;
using InputSource = ::InputSource;
using Serial = ::SerialInput;
using SerialInput = ::SerialInput;
using Joystick = ::JoystickInput;
using JoystickInput = ::JoystickInput;
using JoystickConfig = ::JoystickConfig;
using Buttons = ::ButtonInput;
using ButtonInput = ::ButtonInput;
using ButtonConfig = ::ButtonInputConfig;
using KeyTracker = ::KeyTracker;
} // namespace input

namespace ui {
using Page = ::Page;
using Style = ::Style;
using Length = ::Length;
using Edges = ::Edges;
using Position = ::Position;
using Align = ::Align;
using FontRole = ::FontRole;
using ImageFit = ::ImageFit;
using ButtonColor = ::ButtonColor;
using ButtonVariant = ::ButtonVariant;
using Event = ::UIEvent;
using UIEvent = ::UIEvent;
using Key = ::UIKey;
using UIKey = ::UIKey;
using KeyPhase = ::UIKeyPhase;
using UIKeyPhase = ::UIKeyPhase;
using Node = ::UINode;
using UINode = ::UINode;
using Div = ::UIDiv;
using UIDiv = ::UIDiv;
using Text = ::UIText;
using UIText = ::UIText;
using Image = ::UIImage;
using UIImage = ::UIImage;
using Button = ::UIButton;
using UIButton = ::UIButton;
using Toggle = ::UIToggle;
using UIToggle = ::UIToggle;
using Range = ::UIRange;
using UIRange = ::UIRange;
using Select = ::UISelect;
using UISelect = ::UISelect;
using SelectOption = ::UISelectOption;
} // namespace ui

namespace theme = ::Theme;

} // namespace flow32
