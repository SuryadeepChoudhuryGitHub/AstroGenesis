#pragma once

#include "imgui.h"
#include <string>

namespace AstroGenesis {

enum class IconId {
    None,
    // Branding
    Logo,
    LogoCompact,
    
    // Core Toolbar & Sandbox Actions
    Select,
    Move,
    Add,
    Delete,
    Edit,
    Clone,
    Focus,
    Target,
    Play,
    Pause,
    StepForward,
    StepBackward,
    Reset,
    Undo,
    Redo,
    Search,
    Info,
    Warning,
    Error,
    Success,
    Check,
    CheckCircle,
    Close,
    Save,
    Import,
    Folder,
    Tools,
    Presets,
    Settings,
    Hierarchy,
    Details,
    Filter,
    Maximize,
    Eye,
    EyeSlash,
    Refresh,
    Database,
    AI,

    // Workspaces & Subsystems
    Universe,
    System,
    Objects,
    Explore,
    Simulation,
    AIAssistant,
    DataManager,
    Validation,
    AsteroidBelt,
    MatterLab,
    Cinematic,
    PhotoMode,
    Camera,
    Physics,
    Orbit,
    Time,

    // Celestial Types & Physical Properties
    Star,
    Planet,
    Moon,
    Asteroid,
    Comet,
    BlackHole,
    Matter,
    Energy,
    Speed,
    Ruler,
    Thermometer,
    Pressure,
    Density,
    Gravity,
    Atmosphere,
    Clouds,
    Radiation,
    Terminal,

    // Directional Navigation
    ChevronUp,
    ChevronDown,
    ChevronLeft,
    ChevronRight,

    // Aliases & Extended semantic mappings
    Copy = Clone,
    Heat = Thermometer,
    Phase = Matter,
    Scale = Ruler,
    Sparkles = AI,
    Sun = Star
};

namespace IconSize {
    constexpr float Small    = 13.0f; // Table rows, inline small items, badges
    constexpr float Standard = 16.0f; // Standard buttons, tabs, headers
    constexpr float Medium   = 18.0f; // Important toolbar buttons
    constexpr float Large    = 22.0f; // Prominent action buttons, section cards
    constexpr float Hero     = 30.0f; // Large modals, empty states, hero branding
}

namespace UIThemeCol {
    inline const ImVec4 Accent        {0.00f, 0.85f, 1.00f, 1.00f}; // Cyan/Blue
    inline const ImVec4 AccentDim     {0.00f, 0.50f, 0.70f, 0.70f};
    inline const ImVec4 TextPrimary   {0.92f, 0.95f, 0.98f, 1.00f};
    inline const ImVec4 TextSecondary {0.50f, 0.60f, 0.72f, 1.00f};
    inline const ImVec4 TextMuted     {0.45f, 0.52f, 0.62f, 0.85f};
    inline const ImVec4 Success       {0.15f, 0.88f, 0.45f, 1.00f}; // Emerald Green
    inline const ImVec4 Warning       {0.98f, 0.78f, 0.15f, 1.00f}; // Amber / Gold
    inline const ImVec4 Error         {0.95f, 0.28f, 0.22f, 1.00f}; // Coral Red
    inline const ImVec4 Orange        {0.98f, 0.55f, 0.15f, 1.00f};
    inline const ImVec4 Purple        {0.70f, 0.35f, 0.95f, 1.00f};
    inline const ImVec4 TabActive     {0.00f, 0.50f, 0.75f, 0.35f};
}

namespace UICol = UIThemeCol;

namespace UIIcon {
    // Direct vector drawing on an ImDrawList at a center coordinate
    void Draw(ImDrawList* dl, IconId icon, ImVec2 center, float size, ImU32 color, float thickness = 0.0f);

    // Inline icon item within ImGui flow
    void Icon(IconId icon, float size = IconSize::Standard, ImU32 color = 0, float spacingAfter = 6.0f);
    void Icon(IconId icon, float size, const ImVec4& color, float spacingAfter = 6.0f);

    // Standard button featuring an icon and optional text label
    bool Button(const char* str_id, IconId icon, const char* label = nullptr, 
                const ImVec2& size = ImVec2(0, 0), bool active = false, 
                const ImVec4* customBg = nullptr, const ImVec4* customFg = nullptr);

    // Convenience overload: icon first, label as both ID and text
    bool Button(IconId icon, const char* label, 
                const ImVec2& size = ImVec2(0, 0), bool active = false, 
                const ImVec4* customBg = nullptr, const ImVec4* customFg = nullptr);

    // Compact button with icon (ideal for toolbars, collapse toggles, table rows)
    bool SmallButton(const char* str_id, IconId icon, const char* label = nullptr);
    bool SmallButton(IconId icon, const char* label = nullptr);

    // Clean stat item: vertically centered vector icon + secondary label + primary value
    void StatItem(IconId icon, const char* label, const char* value, float iconSize = IconSize::Small, const ImVec4* iconColor = nullptr);

    // AstroGenesis Visual Identity Branding
    void DrawLogo(ImDrawList* dl, ImVec2 center, float radius, ImU32 accentCol, ImU32 secondaryCol);
    void DrawBranding(float height = 26.0f, bool compact = false);
}

} // namespace AstroGenesis
