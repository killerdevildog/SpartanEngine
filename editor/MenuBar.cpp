/*
Copyright(c) 2015-2025 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES =====================
#include "MenuBar.h"
#include "GeneralWindows.h"
#include "Widgets/Profiler.h"
#include "Widgets/ShaderEditor.h"
#include "Widgets/RenderOptions.h"
#include "Widgets/TextureViewer.h"
#include "Widgets/ResourceViewer.h"
#include "Widgets/AssetBrowser.h"
#include "Widgets/Console.h"
#include "Widgets/Properties.h"
#include "Widgets/Viewport.h"
#include "Widgets/WorldViewer.h"
#include "Widgets/FileDialog.h"
#include "Widgets/Style.h"
#include "Engine.h"
#include "Profiling/RenderDoc.h"
#include "Debugging.h"
//================================

//= NAMESPACES =====
using namespace std;
//==================

namespace
{
    bool show_file_dialog          = false;
    bool show_imgui_metrics_window = false;
    bool show_imgui_style_window   = false;
    bool show_imgui_demo_widow     = false;
    Editor* editor                 = nullptr;
    string file_dialog_selection_path;
    unique_ptr<FileDialog> file_dialog;

    template <class T>
    void menu_entry()
    {
        T* widget = editor->GetWidget<T>();

        // menu item with checkmark based on widget->GetVisible()
        if (ImGui::MenuItem(widget->GetTitle().c_str(), nullptr, widget->GetVisible()))
        {
            // toggle visibility
            widget->SetVisible(!widget->GetVisible());
        }
    }

    namespace windows
    {
        void ShowWorldSaveDialog()
        {
            file_dialog->SetOperation(FileDialog_Op_Save);
            show_file_dialog = true;
        }
        
        void ShowWorldLoadDialog()
        {
            file_dialog->SetOperation(FileDialog_Op_Load);
            show_file_dialog = true;
        }
        
        void DrawFileDialog()
        {
            if (show_file_dialog)
            {
                ImGui::SetNextWindowFocus();
            }
        
            if (file_dialog->Show(&show_file_dialog, editor, nullptr, &file_dialog_selection_path))
            {
                // load world
                if (file_dialog->GetOperation() == FileDialog_Op_Open || file_dialog->GetOperation() == FileDialog_Op_Load)
                {
                    if (spartan::FileSystem::IsEngineSceneFile(file_dialog_selection_path))
                    {
                        spartan::ThreadPool::AddTask([]()
                        {
                            spartan::World::LoadFromFile(file_dialog_selection_path);
                        });

                        show_file_dialog = false;
                    }
                }
                // save world
                else if (file_dialog->GetOperation() == FileDialog_Op_Save)
                {
                    if (file_dialog->GetFilter() == FileDialog_Filter_World)
                    {
                        spartan::ThreadPool::AddTask([]()
                        {
                            spartan::World::SaveToFile(file_dialog_selection_path);
                        });

                        show_file_dialog = false;
                    }
                }
            }
        }
    }

    namespace buttons_menu
    {
        void world()
        {
            if (ImGui::BeginMenu("World"))
            {
                if (ImGui::MenuItem("New"))
                {
                    spartan::World::Clear();
                }

                ImGui::Separator();

                // the engine has changed a lot, so I need to re-write resource cache serialization/deserialization
                // grey out the options so users know that the functionality is part of the engine but currently disabled
                ImGui::BeginDisabled(true);
                {
                    if (ImGui::MenuItem("Load"))
                    {
                        windows::ShowWorldLoadDialog();
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Save", "Ctrl+S"))
                    {
                        windows::ShowWorldSaveDialog();
                    }

                    if (ImGui::MenuItem("Save As...", "Ctrl+S"))
                    {
                        windows::ShowWorldSaveDialog();
                    }
                }
                ImGui::EndDisabled();

                ImGui::EndMenu();
            }
        }

        void view()
        {
            if (ImGui::BeginMenu("View"))
            {
                if (ImGui::MenuItem("Controls", "Ctrl+P", GeneralWindows::GetVisiblityWindowControls()))
                {

                }

                if (ImGui::BeginMenu("Widgets"))
                {
                    menu_entry<Profiler>();
                    menu_entry<ShaderEditor>();
                    menu_entry<RenderOptions>();
                    menu_entry<TextureViewer>();
                    menu_entry<ResourceViewer>();
                    menu_entry<AssetBrowser>();
                    menu_entry<Console>();
                    menu_entry<Properties>();
                    menu_entry<Viewport>();
                    menu_entry<WorldViewer>();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("ImGui"))
                {
                    ImGui::MenuItem("Metrics", nullptr, &show_imgui_metrics_window);
                    ImGui::MenuItem("Style", nullptr, &show_imgui_style_window);
                    ImGui::MenuItem("Demo", nullptr, &show_imgui_demo_widow);

                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }
        }

        void help()
        {
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::MenuItem("About", nullptr, GeneralWindows::GetVisiblityWindowAbout());

                if (ImGui::MenuItem("Sponsor", nullptr, nullptr))
                {
                    spartan::FileSystem::OpenUrl("https://github.com/sponsors/PanosK92");
                }

                if (ImGui::MenuItem("Contributing", nullptr, nullptr))
                {
                    spartan::FileSystem::OpenUrl("https://github.com/PanosK92/SpartanEngine/blob/master/contributing.md");
                }

                if (ImGui::MenuItem("Perks of a contributor", nullptr, nullptr))
                {
                    spartan::FileSystem::OpenUrl("https://github.com/PanosK92/SpartanEngine/wiki/Perks-of-a-contributor");
                }

                if (ImGui::MenuItem("Report a bug", nullptr, nullptr))
                {
                    spartan::FileSystem::OpenUrl("https://github.com/PanosK92/SpartanEngine/issues/new/choose");
                }

                if (ImGui::MenuItem("Join the Discord server", nullptr, nullptr))
                {
                    spartan::FileSystem::OpenUrl("https://discord.gg/TG5r2BS");
                }

                ImGui::EndMenu();
            }
        }
    }

    namespace buttons_toolbar
    {
        float button_size = 19.0f;
        unordered_map<IconType, Widget*> widgets;

        // a button that when pressed will call "on press" and derives it's color (active/inactive) based on "get_visibility".
        void toolbar_button(IconType icon_type, const string tooltip_text, const function<bool()>& get_visibility, const function<void()>& on_press, float cursor_pos_x = -1.0f)
        {
            ImGui::SameLine();
            ImVec4 button_color = get_visibility() ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_Button];
            ImGui::PushStyleColor(ImGuiCol_Button, button_color);
            if (cursor_pos_x > 0.0f)
            {
                ImGui::SetCursorPosX(cursor_pos_x);
            }

            const ImGuiStyle& style   = ImGui::GetStyle();
            const float size_avail_y  = 2.0f * style.FramePadding.y + button_size;
            const float button_size_y = button_size + 2.0f * MenuBar::GetPaddingY();
            const float offset_y      = (button_size_y - size_avail_y) * 0.5f;

            ImGui::SetCursorPosY(offset_y);

            if (ImGuiSp::image_button(nullptr, icon_type, button_size * spartan::Window::GetDpiScale(), false))
            {
                on_press();
            }

            ImGui::PopStyleColor();

            ImGuiSp::tooltip(tooltip_text.c_str());
        }

        void tick()
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const float size_avail_x      = viewport->Size.x;
            const float button_size_final = button_size * spartan::Window::GetDpiScale() + MenuBar::GetPaddingX() * 2.0f;
            float num_buttons             = 1.0f;
            float size_toolbar            = num_buttons * button_size_final;
            float cursor_pos_x            = (size_avail_x - size_toolbar) * 0.5f;

            // play button
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 18.0f, MenuBar::GetPaddingY() - 5.0f });

                toolbar_button(
                    IconType::Button_Play, "Play",
                    []() { return spartan::Engine::IsFlagSet(spartan::EngineMode::Playing);  },
                    []() { return spartan::Engine::ToggleFlag(spartan::EngineMode::Playing); },
                    cursor_pos_x
                );
                
                ImGui::PopStyleVar(1);
            }

            // all the other buttons
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { MenuBar::GetPaddingX() - 1.0f, MenuBar::GetPaddingY() - 5.0f });
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 4.0f , 0.0f });
            {
                num_buttons  = 7.0f;
                size_toolbar = num_buttons * button_size_final + (num_buttons - 1.0f) * ImGui::GetStyle().ItemSpacing.x;
                cursor_pos_x = size_avail_x - (size_toolbar - 2.0f);

                // buttons from custom functionality
                {
                    // renderdoc button
                    toolbar_button(IconType::Button_RenderDoc, "Captures the next frame and then launches RenderDoc",
                        []() { return false; },
                        []()
                        {
                            if (spartan::Debugging::IsRenderdocEnabled())
                            {
                                spartan::RenderDoc::FrameCapture();
                            }
                            else
                            {
                                SP_LOG_WARNING("RenderDoc integration is disabled. To enable, go to \"Debugging.h\", and set \"is_renderdoc_enabled\" to \"true\"");
                            }
                        },
                        cursor_pos_x
                    );

                    // world selection
                    toolbar_button(IconType::Component_Terrain, "World selection window",
                        []() { return GeneralWindows::GetVisibilityWorlds(); },
                        []()
                        {
                            GeneralWindows::SetVisibilityWorlds(!GeneralWindows::GetVisibilityWorlds());
                        }
                    );
                }

                // buttons from widgets
                for (auto& widget_it : widgets)
                {
                    Widget* widget            = widget_it.second;
                    const IconType widget_icon = widget_it.first;

                    toolbar_button(widget_icon, widget->GetTitle(),
                        [&widget]() { return widget->GetVisible(); },
                        [&widget]() { widget->SetVisible(true); }
                    );
                }
            }
            ImGui::PopStyleVar(2);

            // screenshot
            //toolbar_button(
            //    IconType::Screenshot, "Screenshot",
            //    []() { return false; },
            //    []() { return spartan::Renderer::Screenshot("screenshot.png"); }
            //);
        }
    }

    // window buttons like minimize, maximize, close (not used yet)
    namespace buttons_titlebar
    {
        void tick()
        {
            // snap to the right
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            const float size_avail_x      = viewport->Size.x;
            const float offset_right      = 120.0f * spartan::Window::GetDpiScale();
            ImGui::SetCursorPosX(size_avail_x - offset_right);

            spartan::math::Vector2 icon_size = spartan::math::Vector2(24.0f, 24.0f);
            if (ImGuiSp::image_button(nullptr, IconType::Window_Minimize, icon_size, false))
            {
                spartan::Window::Minimize();
            }

            if (ImGuiSp::image_button(nullptr, IconType::Window_Maximize, icon_size, false))
            {
                spartan::Window::Maximize();
            }

            if (ImGuiSp::image_button(nullptr, IconType::Window_Close, icon_size, false))
            {
                spartan::Window::Close();
            }
        }
    }
}

void MenuBar::Initialize(Editor* _editor)
{
    editor      = _editor;
    file_dialog = make_unique<FileDialog>(true, FileDialog_Type_FileSelection, FileDialog_Op_Open, FileDialog_Filter_World);

    buttons_toolbar::widgets[IconType::Button_Profiler]        = editor->GetWidget<Profiler>();
    buttons_toolbar::widgets[IconType::Button_ResourceCache]   = editor->GetWidget<ResourceViewer>();
    buttons_toolbar::widgets[IconType::Button_Shader]          = editor->GetWidget<ShaderEditor>();
    buttons_toolbar::widgets[IconType::Component_Options]      = editor->GetWidget<RenderOptions>();
    buttons_toolbar::widgets[IconType::Directory_File_Texture] = editor->GetWidget<TextureViewer>();

    spartan::Engine::SetFlag(spartan::EngineMode::Playing, false);
}

void MenuBar::Tick()
{
    // menu
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 8.0f));

        if (ImGui::BeginMainMenuBar())
        {
            buttons_menu::world();
            buttons_menu::view();
            buttons_menu::help();
            buttons_toolbar::tick();

            ImGui::EndMainMenuBar();
        }

        ImGui::PopStyleVar();
    }

    // windows
    {
        if (show_imgui_metrics_window)
        {
            ImGui::ShowMetricsWindow();
        }

        if (show_imgui_demo_widow)
        {
            ImGui::ShowDemoWindow(&show_imgui_demo_widow);
        }

        editor->GetWidget<Style>()->SetVisible(show_imgui_style_window);
    }

    windows::DrawFileDialog();
}

void MenuBar::ShowWorldSaveDialog()
{
    windows::ShowWorldSaveDialog();
}

void MenuBar::ShowWorldLoadDialog()
{
    windows::ShowWorldLoadDialog();
}
